all: archlinux.test

%.test:
	docker build -t $* ./$*/ && docker run --rm $* && touch $@

clean:
	rm -f *.test

dockerprune:
	docker system prune -a
