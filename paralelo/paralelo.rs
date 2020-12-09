extern crate libc;
extern crate regex;
extern crate mpi;

use std::borrow::Cow;
use std::cmp;
use std::env;
use std::fs;
use std::io::{self, Read};
use std::ptr::null;
use std::sync::mpsc;

use regex::Regex;

use mpi::request::WaitGuard;
use mpi::datatype::PartitionMut;
use mpi::Count;
use mpi::traits::*;

fn main() {
    let universe = mpi::initialize().unwrap();
    let world = universe.world();
    let comm_sz = world.size();
    let my_rank = world.rank();

    let mut buffer = String::new();

    let mut fasta = vec![0];
    let mut len = 0;
    let mut old_len = 0;

    if my_rank == 0 {
        old_len = io::stdin().read_to_string(&mut buffer).unwrap();

        buffer = Regex::new(">[^\n]*\n|\n").unwrap().replace_all(&buffer,"").to_string();

        len = buffer.len();

        //fasta = buffer.chars().collect();
        let buffer_chars: Vec<char> = buffer.chars().collect();
        fasta = vec![0; len];
        for (pos, e) in buffer_chars.iter().enumerate() {
            fasta[pos] = buffer_chars[pos] as u32;
        }
    }

    world.process_at_rank(0).broadcast_into(&mut len);
    if my_rank != 0 {
        fasta = vec![0; len];
    }

    world.process_at_rank(0).broadcast_into(&mut fasta[..]);

    let mut fasta_chars = vec!['a'; len];
    for (pos, e) in fasta.iter().enumerate() {
        let c = std::char::from_u32(fasta[pos]);
        match c {
            Some(c) => {
                fasta_chars[pos] = c;
            },
            None => ()
        }
    }

    let mut fasta_str: String = fasta_chars.into_iter().collect();

    let to_replace = vec![
        (Regex::new("tHa[Nt]").unwrap(), "<4>"),
        (Regex::new("aND|caN|Ha[DS]|WaS").unwrap(), "<3>"),
        (Regex::new("a[NSt]|BY").unwrap(), "<2>"),
        (Regex::new("<[^>]*>").unwrap(), "|"),
        (Regex::new("\\|[^|][^|]*\\|").unwrap(), "-"),
    ];

    let variants = vec![
        Regex::new("agggtaaa|tttaccct").unwrap(),
        Regex::new("[cgt]gggtaaa|tttaccc[acg]").unwrap(),
        Regex::new("a[act]ggtaaa|tttacc[agt]t").unwrap(),
        Regex::new("ag[act]gtaaa|tttac[agt]ct").unwrap(),
        Regex::new("agg[act]taaa|ttta[agt]cct").unwrap(),
        Regex::new("aggg[acg]aaa|ttt[cgt]ccct").unwrap(),
        Regex::new("agggt[cgt]aa|tt[acg]accct").unwrap(),
        Regex::new("agggta[cgt]a|t[acg]taccct").unwrap(),
        Regex::new("agggtaa[cgt]|[acg]ttaccct").unwrap(),
    ];

    //let ((input_len, text), (to_replace, variants)) : ((usize, String), (Vec<(Regex, &str)>, Vec<Regex>)) =
    //    futures::executor::block_on(future);

    let removed_description_len = fasta_str.len();

    let mut displs = vec![0; comm_sz as usize];
    let mut rec_counts = vec![0; comm_sz as usize];
    let sz = comm_sz as usize;
    for pos in 0..sz {
        let mut n = variants.len() / sz;
        let mut a = pos * n;
        let remainder = variants.len() % sz;
        if remainder > 0 {
            if pos < remainder {
                n += 1;
            }

            a += std::cmp::min(remainder, pos);
        }
        displs[pos] = a;
        rec_counts[pos] = n;
    }

    let local_n = rec_counts[my_rank as usize];
    let local_a = displs[my_rank as usize];
    let local_b = local_a + local_n;
    let mut local_count = vec![0; local_n];
    //println!("{} {} {}", my_rank, local_a, local_b);
    for i in 0..local_n {
        local_count[i] = variants[local_a + i].captures_iter(&fasta_str).count() as i32;
    }

    if my_rank == 0 {
        let final_counts: Vec<Count> = rec_counts.iter().map(|&x| x as i32).collect();
        let final_displs: Vec<Count> = displs.iter().map(|&x| x as i32).collect();

        let mut buf = vec![0; variants.len()];
        {
            let mut partition = PartitionMut::new(&mut buf[..], final_counts, &final_displs[..]);
            world.process_at_rank(0).gather_varcount_into_root(&local_count[..], &mut partition);
        }

        for pos in 0..variants.len() {
            println!("{} {}", variants[pos], buf[pos]);
        }
    } else {
        world.process_at_rank(0).gather_varcount_into(&local_count[..]);
    }

    if my_rank == 0 {
        let mut output = String::from(fasta_str);
        for (r, replacement) in to_replace {
            output = r.replace_all(&output, replacement).to_string();
        }
        let final_len = output.len();

        println!("\n{}\n{}\n{}", old_len, len, final_len);
        //println!("\n{}\n{}\n{:?}", input_len, removed_description_len, sequence_len);
    }
}