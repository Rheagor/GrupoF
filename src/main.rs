extern crate libc;
extern crate rayon;
extern crate regex;

use std::cmp;
use std::env;
use std::io::{self, Read};
use std::ptr::null;
use std::sync::mpsc;

use futures::join;
use rayon::prelude::*;
use regex::Regex;

use self::rayon::{ThreadPool, ThreadPoolBuilder};

fn main() {
    let args: Vec<String> = env::args().collect();
    let file = &args[1];
    println!("{:?}", file);
    println!("starting");
    let pool = ThreadPoolBuilder::new().build().expect("Failed to build pool");

    let future = async move {
        let future1 = async move {
            let mut buffer = String::new();
            let len = io::stdin().read_to_string(&mut buffer).unwrap();

            Regex::new(">[^\n]*\n|\n").unwrap().replace_all(&buffer,"");
            return (len, buffer);
        };

        let future2 = async {
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

            return (to_replace, variants);
        };

        return join!(future1, future2);
    };

    print!("finished join 1");

    let ((input_len, text), (to_replace, variants)) : ((usize, String), (Vec<(Regex, &str)>, Vec<Regex>)) =
        futures::executor::block_on(future);

    let text_len= text.len();
    let t = &text;
    let t2 = &text.clone();

    let f2 = async move {
        let future1 = async move {
            for (r, replacement) in to_replace {
                r.replace_all(t, replacement);
            }

            return (*t).len();
        };

        let future2 = async move {
            let buf = (*t2).clone();
            return variants
                .into_par_iter()
                .map(|variant| {
                    let count = variant.captures_iter(&buf).count();
                    return format!("{} {}", variant.as_str(), count);
                })
                .collect();
        };

        return join!(future1, future2);
    };

    println!("finished join 2");

    let (sequence_len, counts) : (usize, Vec<String>) = futures::executor::block_on(f2);

    for variant in counts {
        println!("{}", variant)
    }
    println!("\n{}\n{}\n{:?}", input_len, text_len, sequence_len);
}