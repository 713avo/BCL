.PHONY: all clean

PDF_NAME = BCL_Extensions_v2.0.pdf

all: $(PDF_NAME)

$(PDF_NAME): main.tex
	pdflatex main.tex
	pdflatex main.tex
	mv main.pdf $(PDF_NAME)

clean:
	rm -f *.aux *.log *.toc *.out main.pdf

cleanall: clean
	rm -f $(PDF_NAME)
