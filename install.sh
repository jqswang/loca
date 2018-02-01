#!/bin/bash

gem install launchy
gem install rest-client
gem install rack

gem/lib/loca-client config


mkdir -p bin
ln -s gem/lib/loca-client bin/loca-client
ln -s server/loca-server bin/loca-server
