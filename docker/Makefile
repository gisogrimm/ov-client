all: ubuntu2104.test ubuntu2110.test debian.test archlinux.test fedora.test

GITBRANCH=$(shell git branch --no-color 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/')

gitbranch:
	echo $(GITBRANCH)

%.test:
	docker build -t $* ./$*/ && docker run --name=$* $* $(GITBRANCH) && docker cp $*:build/ov-client/packaging/deb/debian ./artifacts/ && docker container rm $* && touch $@

clean:
	rm -f *.test

dockerprune:
	docker system prune -a
