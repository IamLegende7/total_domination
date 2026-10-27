# Interchangable File Format (IFF) and custom FORM types

The Maps in Total Domination are stored in the IFF file standard.

TD uses custom FORM types for this purpose:

- CHNK: one 16x16 section of the map
- ACTR: an actor (= building or unit) and the position, where it is to be placed
- TLID & ACID: lists of ID strings, to be refrenced by either tiles in CHNK chunks or actors in ACTR chunks
- META: the map metadata

DATA and ACTS are just arrays of CHNK or ACTR chunks.

**REMEMBER**: IFF is big-endian!

**Note**: CHNK and ACTR always have a size of 768 (h300) and 20 (h14) respectively. The size of IDRF varies.

## CHNK: a Map CHuNK

```
Offset Size Example      Meaning
------ ---- -----------  ------------------------
0000   4    43 48 4E 4B  Label ("CHNK")
0004   4    00 00 06 00  Chunk size

0008   1    00           Tile0 top_tile index
0009   1    01           Tile0 base_tile index
000A   1    01           Tile0 height

000B   1    00           Tile1 top_tile index
000C   1    01           Tile1 base_tile index
000D   1    01           Tile1 height

...

0302   1    00           Tile254 top_tile index
0303   1    01           Tile254 base_tile index
0304   1    01           Tile254 height

0305   1    00           Tile255 top_tile index
0306   1    01           Tile255 base_tile index
0307   1    01           Tile255 height
```

The offset of a tile is easily computable using: `offset=((y*16)+x)*3` (and +8 for the chunk header)

The offset of a chunk can be calculated using: `offset=((chunk_y*cols)+chunk_x)*776`

## ACTR: a saved ACToR

```
Offset Size Example      Meaning
------ ---- -----------  ---------------------------------
0000   4    41 43 54 52  Label ("ACTR")
0004   4    00 00 00 14  Chunk size

0008   2    01           Actor id index
000A   4    00 00 00 00  x
000E   4    00 00 00 00  y
0012   1    00           owner
0013   2    FF FF        max hp (FFFFFFFF = default value)
0015   2    FF FF        hp
0017   2    FF FF        movement speed
0019   2    FF FF        attack value
001B   2    FF FF        defence value
```

## TLID: TiLe IDs

```
Offset Size Example                  Meaning
------ ---- -----------------------  ---------------------------
0000   4    49 44 52 46              Label ("TLID")
0004   4    00 00 00 08              Chunk size

0008   N    74 64 3A 74 65 73 74 00  id string (null-terminated)

...
```

## ACID: ACtor IDs

```
Offset Size Example                  Meaning
------ ---- -----------------------  ---------------------------
0000   4    49 44 52 46              Label ("ACID")
0004   4    00 00 00 08              Chunk size

0008   N    74 64 3A 74 65 73 74 00  id string (null-terminated)

...
```

## META

(all strings are null-terminated)

```
Type       Meaning
---------  ---------------------------
IFF label  Label ("META")
32 int     Chunk size

32 uint    Version
string     Author
string     Creation date as yyyy-mm-dd
string     Map name
string     Map description
32 uint    Rows
32 uint    Cols
```

# IFF Map file structure

A Map for Total Domination should use the following structure:

```
FORM TDMP
 ├── META
 ├── TLID
 ├── ACID
 ├── FORM DATA
 |    ├── CHNK
 |    ...
 |    └── CHNK
 └── FORM ACTS
      ├── ACTR
      ...
      └── ACTR
```