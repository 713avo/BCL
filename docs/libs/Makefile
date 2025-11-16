.PHONY: all clean

all: main.pdf

main.pdf: main.tex chapters/*.tex
	pdflatex main.tex
	makeindex main.idx
	pdflatex main.tex
	pdflatex main.tex

clean:
	rm -f *.aux *.log *.toc *.idx *.ilg *.ind *.out *.pdf
	rm -f chapters/*.aux
