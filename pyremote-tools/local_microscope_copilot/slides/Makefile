TEX = local-microscope-copilot.tex
PDF = local-microscope-copilot.pdf
LOGO = ../../../icons/gxsm4-icon.svg
BACKGROUNDS = backgrounds/content-background.jpeg backgrounds/dark-background.jpeg
FIGURES = figures/meteorite-molecules.jpg figures/bnl-story-qr.png

.PHONY: all clean

all: $(PDF)

$(PDF): $(TEX) $(LOGO) $(BACKGROUNDS) $(FIGURES)
	pdflatex --shell-escape -interaction=nonstopmode -halt-on-error $(TEX)
	pdflatex --shell-escape -interaction=nonstopmode -halt-on-error $(TEX)

clean:
	rm -f $(PDF) local-microscope-copilot.aux local-microscope-copilot.fdb_latexmk \
		local-microscope-copilot.fls local-microscope-copilot.log \
		local-microscope-copilot.nav local-microscope-copilot.out \
		local-microscope-copilot.snm local-microscope-copilot.synctex.gz \
		local-microscope-copilot.toc svg-inkscape/gxsm4-icon_svg-raw.pdf
	rmdir svg-inkscape 2>/dev/null || true
