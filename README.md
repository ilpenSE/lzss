# LZSS Encoder/Decoder

- This C program can encode files using LZSS with simple format. It even have no header or additional meta-info. Just literals, lengths, distances and seperator bit between them

## Quick Start

- Take a build: (Add `r=1` to have release one)
```bash
make
```

- Run and test the program:
```bash
./build/lzss --help # For help
./build/lzss main.c -e -o main.c.lzss_encoded # An example of encoding a file
./build/lzss main.c.lzss_encoded -d -o main.c.lzss_decoded # An example of decoding a file
diff main.c main.c.lzss_decoded # Should be empty if it's latest build
```

## File Format

- We use [bw.h](https://github.com/ilpenSE/bw.h.git) to simplify bit operations.
- And [str.h](https://github.com/ilpenSE/str.h.git) to have modern string implementation in C.

- There're NO headers or footers.
- In encoded binary, we've "block"s of symbols they can be literal type or "match" type (distance-length pair)
- If it's literal, the first bit will be 1. 0 if it's match.
- Then if it's literal, UTF-8 representation of that character comes and block ends. (No trailing bit or byte)
- Or if it's a match, first 8 bits are 'length' - 3 (We've substracted 3 to fit length into uint8_t) then coming 16 bits are 'distance'. Block ends afterwards.
- Read [LZSS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski) algorithm to understand length and distance concepts.

## References

- LZSS Algorithm: https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski
- bw.h: https://github.com/ilpenSE/bw.h.git
- str.h: https://github.com/ilpenSE/str.h.git
