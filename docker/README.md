# Test build on archlinux

````
docker build -t archlinux ./archlinux/ && docker run --rm archlinux
````

# Test build on Ubuntu 21.04

````
docker build -t ubuntu2104 ./ubuntu2104/ && docker run --rm archlinux
````

# Test build on Fedora

````
docker build -t fedora ./fedora/ && docker run --rm fedora
````

# other systems

See Dockerfiles for more information.
