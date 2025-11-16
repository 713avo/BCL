.PHONY: all clean cleanall

PDF_NAME = BCL_Standard_Libraries_v2.0.pdf

all: $(PDF_NAME)

$(PDF_NAME): main.tex chapters/*.tex
	pdflatex main.tex
	makeindex main.idx
	pdflatex main.tex
	pdflatex main.tex
	mv main.pdf $(PDF_NAME)

clean:
	rm -f *.aux *.log *.toc *.idx *.ilg *.ind *.out main.pdf
	rm -f chapters/*.aux

cleanall: clean
	rm -f $(PDF_NAME)
