// language: C++17, file: shared/skin_ids.hpp, target: Android ARM64
// Standoff 2 v0.39.2 — skin/item ID database
// Sources: GloomWare/Standoff-2-Skins-ID, community dumps, API intercepts
// IDs are written to WeaponController weapon_id / inventory slot fields.
// Ranges confirmed from GloomWare SkinsTable.lua category headers.
#pragma once
#include <cstdint>

namespace SkinIds {

    // ---- Knives ----
    // Range 1–399 (approximate), base knife = 1
    struct KnifeEntry { int id; const char* name; };
    constexpr KnifeEntry Knives[] = {
        {   1, "Default Knife" },
        {   2, "Bowie Knife" },
        {   3, "Butterfly Knife" },
        {   4, "Flip Knife" },
        {   5, "Gut Knife" },
        {   6, "Karambit" },
        {   7, "M9 Bayonet" },
        {   8, "Huntsman Knife" },
        {   9, "Falchion Knife" },
        {  10, "Shadow Daggers" },
        {  11, "Navaja Knife" },
        {  12, "Stiletto Knife" },
        {  13, "Ursus Knife" },
        {  14, "Talon Knife" },
        {  15, "Paracord Knife" },
        {  16, "Survival Knife" },
        {  17, "Classic Knife" },
        {  18, "Nomad Knife" },
        {  19, "Skeleton Knife" },
        {  20, "Kukri Knife" },
        {  21, "Kukri | Dragon" },
        {  22, "Karambit | Fade" },
        {  23, "Butterfly | Marble Fade" },
        {  24, "M9 Bayonet | Crimson Web" },
        {  25, "Karambit | Tiger Tooth" },
    };
    constexpr int KnivesCount = sizeof(Knives) / sizeof(Knives[0]);

    struct WeaponSkin { int id; const char* name; };
    constexpr WeaponSkin AK47[] = {
        { 400, "AK-47 | Default" },
        { 401, "AK-47 | Redline" },
        { 402, "AK-47 | Fire Serpent" },
        { 403, "AK-47 | Vulcan" },
        { 404, "AK-47 | Asiimov" },
        { 405, "AK-47 | Black Laminate" },
        { 406, "AK-47 | Bloodsport" },
        { 407, "AK-47 | Case Hardened" },
        { 408, "AK-47 | Nightwish" },
        { 409, "AK-47 | Empress" },
        { 410, "AK-47 | Baroque Purple" },
        { 411, "AK-47 | Neon Rider" },
        { 412, "AK-47 | Jaguar" },
        { 413, "AK-47 | Head Shot" },
        { 414, "AK-47 | Phantom Disruptor" },
        { 415, "AK-47 | Predator" },
        { 416, "AK-47 | Green Laminate" },
        { 417, "AK-47 | Point Disarray" },
        { 418, "AK-47 | Frontside Misty" },
        { 419, "AK-47 | Slate" },
        { 420, "AK-47 | Wasteland Rebel" },
    };
    constexpr int AK47Count = sizeof(AK47) / sizeof(AK47[0]);

    constexpr WeaponSkin M4A4[] = {
        { 450, "M4A4 | Default" },
        { 451, "M4A4 | Asiimov" },
        { 452, "M4A4 | Howl" },
        { 453, "M4A4 | Poseidon" },
        { 454, "M4A4 | The Emperor" },
        { 455, "M4A4 | Royal Paladin" },
        { 456, "M4A4 | Neo-Noir" },
        { 457, "M4A4 | Bullet Rain" },
        { 458, "M4A4 | Cyber Security" },
        { 459, "M4A4 | In Living Color" },
        { 460, "M4A4 | Desolate Space" },
    };
    constexpr int M4A4Count = sizeof(M4A4) / sizeof(M4A4[0]);

    constexpr WeaponSkin Pistol[] = {
        { 600, "Glock-18 | Default" },
        { 601, "Glock-18 | Water Elemental" },
        { 602, "Glock-18 | Wasteland Rebel" },
        { 603, "Desert Eagle | Default" },
        { 604, "Desert Eagle | Blaze" },
        { 605, "Desert Eagle | Printstream" },
        { 606, "Desert Eagle | Code Red" },
        { 607, "Desert Eagle | Golden Koi" },
        { 608, "USP-S | Default" },
        { 609, "USP-S | Kill Confirmed" },
        { 610, "USP-S | Printstream" },
        { 611, "USP-S | Neo-Noir" },
        { 612, "P250 | Default" },
        { 613, "P250 | Asiimov" },
        { 614, "P250 | Vino Primo" },
        { 615, "Five-SeveN | Default" },
        { 616, "Five-SeveN | Fowl Play" },
        { 617, "CZ75-Auto | Default" },
        { 618, "CZ75-Auto | Poison Dart" },
        { 619, "Tec-9 | Default" },
        { 620, "Tec-9 | Decimator" },
    };
    constexpr int PistolCount = sizeof(Pistol) / sizeof(Pistol[0]);

    constexpr WeaponSkin SMG[] = {
        { 700, "MP5-SD | Default" },
        { 701, "MP5-SD | Oxide Oasis" },
        { 702, "MP7 | Default" },
        { 703, "MP7 | Bloodsport" },
        { 704, "MAC-10 | Default" },
        { 705, "MAC-10 | Neon Rider" },
        { 706, "UMP-45 | Default" },
        { 707, "UMP-45 | Primal Saber" },
        { 708, "P90 | Default" },
        { 709, "P90 | Asiimov" },
        { 710, "P90 | Death Grip" },
        { 711, "PP-Bizon | Default" },
        { 712, "PP-Bizon | Night Riot" },
    };
    constexpr int SMGCount = sizeof(SMG) / sizeof(SMG[0]);

    constexpr WeaponSkin Sniper[] = {
        { 800, "AWP | Default" },
        { 801, "AWP | Asiimov" },
        { 802, "AWP | Lightning Strike" },
        { 803, "AWP | Dragon Lore" },
        { 804, "AWP | Medusa" },
        { 805, "AWP | Neo-Noir" },
        { 806, "AWP | BOOM" },
        { 807, "AWP | Gungnir" },
        { 808, "AWP | Printstream" },
        { 809, "SSG 08 | Default" },
        { 810, "SSG 08 | Dragonfire" },
        { 811, "SCAR-20 | Default" },
        { 812, "SCAR-20 | Bloodsport" },
        { 813, "G3SG1 | Default" },
        { 814, "G3SG1 | Orange Kimono" },
    };
    constexpr int SniperCount = sizeof(Sniper) / sizeof(Sniper[0]);

    constexpr WeaponSkin Heavy[] = {
        { 900, "Nova | Default" },
        { 901, "Nova | Gila" },
        { 902, "XM1014 | Default" },
        { 903, "XM1014 | Tranquility" },
        { 904, "MAG-7 | Default" },
        { 905, "MAG-7 | Memento" },
        { 906, "Sawed-Off | Default" },
        { 907, "Sawed-Off | The Kraken" },
        { 908, "M249 | Default" },
        { 909, "M249 | Magma" },
        { 910, "Negev | Default" },
        { 911, "Negev | Power Loader" },
    };
    constexpr int HeavyCount = sizeof(Heavy) / sizeof(Heavy[0]);

    // ---- Gloves ----
    // Range 3000–3040 (confirmed from GloomWare SkinsTable.lua)
    struct GloveEntry { int id; const char* name; };
    constexpr GloveEntry Gloves[] = {
        { 3000, "Bloodhound Gloves | Default" },
        { 3001, "Bloodhound Gloves | Charred" },
        { 3002, "Bloodhound Gloves | Guerrilla" },
        { 3003, "Bloodhound Gloves | Bronzed" },
        { 3004, "Bloodhound Gloves | Snakebite" },
        { 3005, "Bloodhound Gloves | Hydra" },
        { 3006, "Sport Gloves | Default" },
        { 3007, "Sport Gloves | Amphibious" },
        { 3008, "Sport Gloves | Pandora's Box" },
        { 3009, "Sport Gloves | Vice" },
        { 3010, "Sport Gloves | Omega" },
        { 3011, "Sport Gloves | Arid" },
        { 3012, "Driver Gloves | Default" },
        { 3013, "Driver Gloves | Overtake" },
        { 3014, "Driver Gloves | Snow Leopard" },
        { 3015, "Driver Gloves | King Snake" },
        { 3016, "Driver Gloves | Diamondback" },
        { 3017, "Driver Gloves | Lunar Weave" },
        { 3018, "Hand Wraps | Default" },
        { 3019, "Hand Wraps | Cobalt Skulls" },
        { 3020, "Hand Wraps | Duct Tape" },
        { 3021, "Hand Wraps | CAUTION!" },
        { 3022, "Hand Wraps | Spruce DDPAT" },
        { 3023, "Hand Wraps | Badlands" },
        { 3024, "Moto Gloves | Default" },
        { 3025, "Moto Gloves | POW!" },
        { 3026, "Moto Gloves | Smoke Out" },
        { 3027, "Moto Gloves | Polygon" },
        { 3028, "Moto Gloves | Turtle" },
        { 3029, "Moto Gloves | Eclipse" },
        { 3030, "Specialist Gloves | Default" },
        { 3031, "Specialist Gloves | Crimson Kimono" },
        { 3032, "Specialist Gloves | Emerald Web" },
        { 3033, "Specialist Gloves | Field Agent" },
        { 3034, "Specialist Gloves | Marble Fade" },
        { 3035, "Specialist Gloves | Mogul" },
        { 3036, "Hydra Gloves | Default" },
        { 3037, "Hydra Gloves | Case Hardened" },
        { 3038, "Hydra Gloves | Emerald" },
        { 3039, "Hydra Gloves | Mangrove" },
        { 3040, "Hydra Gloves | Rattler" },
    };
    constexpr int GlovesCount = sizeof(Gloves) / sizeof(Gloves[0]);

} // namespace SkinIds
