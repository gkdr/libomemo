# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [UNRELEASED]
### Added
- It is now possible to add a key with a `prekey` attribute. ([#28](https://github.com/gkdr/libomemo/issues/28))
- A function to check via the attribute whether a received key is a prekey.

### Fixed
- Added missing symlinks for the `.so` files. ([#34](https://github.com/gkdr/libomemo/pull/34)) (thanks, [@hartwork](https://github.com/hartwork)!)

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
