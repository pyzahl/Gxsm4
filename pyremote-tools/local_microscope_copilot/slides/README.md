# Project introduction slides

Rebuild the Beamer PDF from this directory with:

```sh
make
```

The deck follows the paired light/dark background style of the AVS 2026
Beamer deck. Its background images are bundled under `backgrounds/`; the title
slide uses the GXSM4 SVG logo from the repository's `icons/` folder. Inkscape
and the LaTeX `svg` package are required for compilation.

The closing slide includes the meteorite-molecule image credited to Edan
Schultz / National MagLab, via Brookhaven National Laboratory, and links to the
original [BNL story](https://www.bnl.gov/newsroom/news.php?a=123172) with a
scannable QR code.

Remove generated PDF and LaTeX files with `make clean`.
