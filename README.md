# mod-gearscore

An AzerothCore module to calculate and store players' GearScore in the character database. It uses identical formulas to the TacoTip addon to ensure accuracy.

## Features
- Automatically calculates GearScore on login, equip, and unequip events.
- Stores the score in a dedicated `character_gearscore` database table (with class and name).
- Includes an in-game `.gs` command to check your GS or the GS of another player (both online and offline).
- Built-in bot protection: skips calculations for bot accounts to save CPU cycles and database space (configurable).
- Includes setting to toggle CSV exports (upcoming feature via external API).
- Configurable settings via `gearscore.conf`.

## In-Game Commands
- `.gs` - Shows your own GearScore or the GS of your currently selected target. Will show an error if an NPC or a Bot is selected (if bots are disabled).
- `.gs [PlayerName]` - Shows the GearScore of the specified player (pulls from database if offline).
- `.gs help` - Shows the command help message.

## Installation
1. Place this directory (`mod-gearscore`) inside your `modules/` folder.
2. Re-run cmake and compile your server.
3. Import the SQL file located in `sql/characters/base/character_gearscore.sql` into your `acore_characters` database.
4. Copy `conf/gearscore.conf.dist` to `conf/gearscore.conf` and adjust if necessary.

## API / Export (Google Sheets)
Because the GearScore data is stored cleanly inside the `character_gearscore` table, it is extremely easy to export this data via an external script (e.g. PHP or Python) for use in Google Sheets or any other web service.
