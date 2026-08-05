# Third-party notices

Release binary includes Rust crates under these SPDX licenses:

- MIT OR Apache-2.0: `anstream`, `anstyle`, `anstyle-parse`, `anstyle-query`, `block-buffer`, `cfg-if`, `clap`, `clap_builder`, `clap_derive`, `clap_lex`, `colorchoice`, `cpufeatures`, `crypto-common`, `digest`, `equivalent`, `hashbrown`, `heck`, `indexmap`, `is_terminal_polyfill`, `itoa`, `libc`, `proc-macro2`, `quote`, `serde`, `serde_core`, `serde_derive`, `serde_json`, `serde_spanned`, `sha2`, `syn`, `thiserror`, `thiserror-impl`, `toml`, `toml_datetime`, `toml_edit`, `toml_write`, `typenum`, `utf8parse`, `version_check`.
- MIT: `generic-array`, `strsim`, `toon-format`, `winnow`, `zmij`.
- MIT OR Unlicense: `memchr`.
- MIT OR Apache-2.0 OR Unicode-3.0: `unicode-ident`.

Exact versions and checksums are recorded in bundled source `Cargo.lock`; release CI validates licenses with `cargo deny`. Unreal Engine is not bundled or redistributed. Repository-owned native plugin source is embedded in CLI binary and installed only through explicit project setup.
