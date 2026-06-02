// This program parses a CSV from stdin, and prints a report (this matches the report on bench/haskell/ParseCSV.hs)
use std::error::Error;
use std::io;

fn run() -> Result<(), Box<dyn Error>> {
    let stdin = io::stdin();
    let mut reader = csv::Reader::from_reader(stdin.lock());

    let headers = reader.headers()?.clone();
    let header_count = headers.len();
    let mut row_count = 0usize;

    println!("Headers:");
    for header in &headers {
        println!("  - {header}");
    }
    println!();

    for result in reader.records() {
        let record = result?;
        if row_count < 2 {
            println!("Row {}", row_count + 1);
            for index in 0..header_count {
                let header = headers.get(index).unwrap_or("");
                let field = record.get(index).unwrap_or("");
                println!("  {header}: {field}");
            }
            println!();
        }
        row_count += 1;
    }

    println!("Parsed {row_count} data rows.");

    Ok(())
}

fn main() {
    if let Err(err) = run() {
        eprintln!("error: {err}");
        std::process::exit(1);
    }
}
