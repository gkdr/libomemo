# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [UNRELEASED]
### Added
- In some error cases additional information is printed to `stderr` if `LIBOMEMO_DEBUG` is set ([#40](https://github.com/gkdr/libomemo/pull/40))

### Fixed
- Fix default prefix to be `/usr/local` rather than `/` ([#38](https://github.com/gkdr/libomemo/pull/38)) (thanks, [@hartwork](https://github.com/hartwork)!)
- Fix filed `Requires.private` and `Requires` in auto-generated pkg-config file `libomemo.pc` ([#38](https://github.com/gkdr/libomemo/pull/38)) (thanks, [@hartwork](https://github.com/hartwork)!)

### Changed
- Migrate build system from a Makefile to CMake ([#38](https://github.com/gkdr/libomemo/pull/38)) (thanks, [@hartwork](https://github.com/hartwork)!)
- Most XML parsing errors should have a discernible error code now, mostly replacing the more general ones ([#40](https://github.com/gkdr/libomemo/pull/40))

### Infrastructure
- Cover Windows build by GitHub Actions CI using msys2 ([#38](https://github.com/gkdr/libomemo/pull/38)) (thanks, [@hartwork](https://github.com/hartwork)!)
- dependabot: update actions/checkout from 2.4.0 to 3.0.0 ([#41](https://github.com/gkdr/libomemo/pull/41))
- dependabot: update actions/upload-artifact from 2.3.1 to 3.0.0 ([#42](https://github.com/gkdr/libomemo/pull/42))

## [0.8.0] - 2022-02-14
### Added
- It is now possible to add a key with a `prekey` attribute. ([#28](https://github.com/gkdr/libomemo/issues/28))
- A function to check via the attribute whether a received key is a prekey.
- Mention in the README the exact version implemented. ([#26](https://github.com/gkdr/libomemo/issues/26))

### Removed
- It is not any longer possible to set the used XML namespace at build time. ([#21](https://github.com/gkdr/libomemo/issues/21))

### Fixed
- Added missing symlinks for the `.so` files. ([#34](https://github.com/gkdr/libomemo/pull/34)) (thanks, [@hartwork](https://github.com/hartwork)!)
- Fix crossbuild using wrong multiarch triplet. ([#36](https://github.com/gkdr/libomemo/pull/36)) (thanks, [@fortysixandtwo](https://github.com/fortysixandtwo)!)
- Flaky test `test_aes_gcm_encrypt_decrypt`. ([#39](https://github.com/gkdr/libomemo/issues/39)) (thanks, [@hartwork](https://github.com/hartwork)!)

### Infrastructure
- Cover Linux build by GitHub Actions CI ([#37](https://github.com/gkdr/libomemo/pull/37)) (thanks, [@hartwork](https://github.com/hartwork)!)

## [0.7.1] - 2021-01-31
### Fixed
- Test file cleanup now won't randomly break parallel builds. ([#31](https://github.com/gkdr/libomemo/pull/31)) (thanks, [@fortysixandtwo](https://github.com/fortysixandtwo)!)

## [0.7.0] - 2020-12-02
### Added
- This file.
- Makefile target for building a shared library. ([#30](https://github.com/gkdr/libomemo/pull/30)) (thanks, [@fortysixandtwo](https://github.com/fortysixandtwo)!)
- `.vscode` dir to `.gitignore`

### Changed
- The build now includes debug symbols.
- Generated IVs for outgoing messages are now 12 bytes long. This should help compatibility with Monal. ([#24](https://github.com/gkdr/libomemo/issues/24)) (thanks, everyone!)

### Fixed
- `omemo_message_create()` error handling ([#22](https://github.com/gkdr/libomemo/pull/22)) (thanks, [@msantos](https://github.com/msantos)!)
- Wrongly expecting `<key>` and `<iv>` elements to be in a certain order. This should help compatibility with Dino.
- The link to _gcovr_ in the README. ([#29](https://github.com/gkdr/libomemo/pull/29)) (thanks, [@alucaes](https://github.com/aluaces)!)

## [0.6.2] and below
lost to commit messages
