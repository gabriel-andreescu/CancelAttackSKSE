# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [2.0.0] - 2026-09-20

### Added

- Add a custom cancel binding
- Add an optional additional cancel binding
- Add cancel-only mode
- Add weapon-type and dual-wield filters
- Add unarmed attack cancellation
- Set both the start and end of the cancel window
- Add an optional MCM addon

### Changed

- **Breaking change:** Replace the old control settings with a main and optional
  additional binding for Block, Ready Weapon or a custom key
- **Breaking change:** Move settings to `MCM/Settings/CancelAttackSKSE.ini`

### Fixed

- Support Skyrim 1.7.104
- Start blocking with the same press that cancels the attack
- Reset the cancel window for each swing in a combo
- Respect God Mode when applying stamina costs

## [1.3.0] - 2025-06-16

### Added

- Cancel attacks with the Ready Weapon control

## [1.2.0]

### Changed

- Follow the mapped Block control for the current input device

## [1.1.0]

### Added

- Configurable cancel windows and stamina costs by weapon type

## [1.0.1]

### Fixed

- Leave left-hand attacks alone when the right hand has a spell or no weapon

## [1.0.0] - 2025-06-07

### Added

- Initial release
