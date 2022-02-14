#!/bin/sh

# install brew
if which brew; then
    echo "homebrew is already installed"
else
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# install jack
if which jackd; then
    echo "jack is already installed"
else
    brew install jack
fi

# install node
if which node; then
    echo "node is already installed"
else
    brew install node
fi

