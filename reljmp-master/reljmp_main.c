/*
 * Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
 *
 * Author: Juerg Haefliger <juerg.haefliger@hp.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>

#include "reljmp.h"

static int verbose = 1;
module_param(verbose, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(verbose, "Verbose output (default = 1)");

extern struct reljmp rj_printk;

/* List of functions to replace */
struct reljmp *reljmp_func[] = {
	&rj_printk,
};

static int __init reljmp_module_init(void)
{
	int retval;
	int i;

	/* Initialize the jumps */
	retval = reljmp_init_once();
	if (retval) {
		return retval;
	}
	for (i = 0; i < ARRAY_SIZE(reljmp_func); i++) {
		retval = reljmp_init(reljmp_func[i]);
		if (retval) {
			return retval;
		}
	}

	/* Register the jumps */
	for (i = 0; i < ARRAY_SIZE(reljmp_func); i++) {
		reljmp_register(reljmp_func[i], verbose);
	}

	printk("reljmp: module loaded\n");
	return 0;
}

static void __exit reljmp_module_exit(void)
{
	int i;

	/* Unregister the jumps */
	for (i = 0; i < ARRAY_SIZE(reljmp_func); i++) {
		reljmp_unregister(reljmp_func[i], verbose);
	}

	printk("reljmp: module unloaded\n");
}

MODULE_AUTHOR("Juerg Haefliger <juerg.haefliger@hp.com>");
MODULE_DESCRIPTION("Replace kernel functions");
MODULE_LICENSE("GPL");

module_init(reljmp_module_init);
module_exit(reljmp_module_exit);
