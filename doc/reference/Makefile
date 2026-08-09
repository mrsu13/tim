.PHONY: all doxygen pdf docx clean changelog dots puml copy-img \
        install-deps install-deps-html install-deps-pdf last-modified

# Имя пользователя для дат: PST оставлено как было в исходном Makefile,
# чтобы метки совпадали с уже сгенерированными артефактами.
timestamp := $(shell TZ=PST git log -1 --remove-empty --no-merges --format='%cd' --date="format-local:%Y%m%d" .)

# Версия TIM подставляется в Doxyfile как $(TIM_VERSION).
export TIM_VERSION := $(shell tr -d '" \n' < ../../VERSION)

# PlantUML: команда из пакета (apt install plantuml). Можно переопределить
# через `make puml PLANTUML='java -jar /path/to/plantuml.jar'`.
PLANTUML ?= plantuml


all: doxygen


# Полный набор зависимостей: и для HTML, и для PDF. Сохранён для
# обратной совместимости; ставит ~1ГБ texlive.
install-deps: install-deps-html install-deps-pdf

# Минимум для HTML-документации.
install-deps-html:
	sudo apt install -y doxygen doxygen-gui graphviz plantuml

# Дополнительные пакеты для сборки PDF / DOCX через XeLaTeX и pandoc.
install-deps-pdf:
	sudo apt install -y build-essential clang \
	                    pandoc pandoc-plantuml-filter \
	                    texlive-xetex \
	                    texlive-fonts-extra texlive-fonts-recommended \
	                    texlive-lang-cyrillic texlive-latex-extra \
	                    texlive-pstricks texlive-science \
	                    fonts-dejavu fonts-cmu


doxygen: dots puml copy-img
	@rm -rf html
	doxygen

pdf-pandoc:	changelog dots puml
	pandoc -F pandoc-plantuml --resource-path=md -f markdown+smart md/*.md -o tim-$(timestamp).pdf

pdf: changelog puml dots copy-img last-modified
	@rm -rf html latex
	doxygen
	cp tex/tim.sty latex/
	cd latex; $(MAKE) LATEX_CMD=xelatex; mv refman.pdf ../tim-$(timestamp).pdf

docx:	changelog dots puml
	pandoc -F pandoc-plantuml --resource-path=md -f markdown+smart md/*.md -o tim-$(timestamp).docx

last-modified:
	$(shell env TZ=PST git log -1 --remove-empty --no-merges --format='%cd' --date="format:%B %d %Y" . > last-modified.tex)

dots:
	if [ ! -d html/images ]; then \
		mkdir -p html/images; \
	fi
	for i in `find dot -name '*.dot'`; \
	do \
		j=html/images/`basename $$i`.png; \
		dot -Tpng "$$i" -o$$j; \
	done

puml:
	if [ ! -d html/images ]; then \
		mkdir -p html/images; \
	fi
	for i in `find puml -name '*.puml' 2>/dev/null`; \
	do \
		$(PLANTUML) -tpng "$$i" -o../html/images; \
	done

copy-img:
	mkdir -p html/images
	for i in `find md/images -type f 2>/dev/null`; \
	do \
		cp "$$i" html/images/; \
	done

changelog:
	echo '\\newpage' > md/01-changelog.md
	echo >> md/01-changelog.md
	echo "# Document Revisions" >> md/01-changelog.md
	echo "table: Document Revisions" >> md/01-changelog.md
	echo >> md/01-changelog.md
	echo "| **Date**               | **Author** | **Comments** |" >> md/01-changelog.md
	echo "| ----------------------------- | -------------------- | ----------------------------------------------- |" >> md/01-changelog.md
	git log --no-merges --date-order --format="| %cd | %cN | %s |" --date="format-local:%d/%m/%y %I:%M %p %Z" . >> md/01-changelog.md

clean:
	rm -rf html latex plantuml-images *.pdf *.epub *.docx md/01-changelog.md *.log tex/*.aux tex/*.fdb_latexmk tex/*.fls tex/*.log symbols.sql last-modified.tex doxy.log Doxyfile.bak *.aux
