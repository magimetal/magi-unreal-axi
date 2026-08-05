use std::process::ExitCode;

fn main() -> ExitCode {
    magi_unreal_axi::run(std::env::args_os())
}
