# reljmp: A simple module to replace kernel functions
#
# Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
#
# Author: Juerg Haefliger <juerg.haefliger@hp.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

NAME = reljmp
$(NAME)-y = $(NAME)_main.o $(NAME)_core.o $(NAME)_printk.o
obj-m = $(NAME).o

KERNEL = $(shell uname -r)

$(NAME):
	$(MAKE) -C /lib/modules/$(KERNEL)/build M=$$PWD

clean:
	$(MAKE) -C /lib/modules/$(KERNEL)/build M=$$PWD clean
	-rm -rf *~ 2>/dev/null || true

load:
	sudo insmod ./$(NAME).ko

unload:
	sudo rmmod $(NAME)

reload:	unload load
