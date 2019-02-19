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

#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>

#include "reljmp.h"

static struct mutex *reljmp_jump_label_mutex = NULL;
static struct mutex *reljmp_text_mutex = NULL;
static void *(*reljmp_text_poke)(void *, const void *, size_t) = NULL;

/* Dump some information to syslog */
static void reljmp_info(char *prefix, const char *name, void *addr)
{
#define BUF_LEN 128
	char buf[BUF_LEN];
	int len, i;

	len = snprintf(buf, BUF_LEN, "%s%s @ 0x%p: (", prefix, name, addr);
	for (i = 0; i < RELATIVEJUMP_SIZE; i++) {
		len += snprintf(buf + len, BUF_LEN - len, "%s%02x",
				(i > 0) ? " " : "" ,
				*((unsigned char *)addr+i));
	}
	len += snprintf(buf + len, BUF_LEN - len, ")");

	printk(KERN_INFO "%s\n", buf);
}

/* Do all the necessary locking before modifying the code */
static inline void reljmp_lock(void)
{
	preempt_disable();
	synchronize_sched();
	mutex_lock(reljmp_jump_label_mutex);
	get_online_cpus();
	mutex_lock(reljmp_text_mutex);
}

/* Do all the necessary unlocking after modifying the code */
static inline void reljmp_unlock(void)
{
	mutex_unlock(reljmp_text_mutex);
        put_online_cpus();
	mutex_unlock(reljmp_jump_label_mutex);
	preempt_enable();
}

/* Remove the relative jump instruction from the begining of the original
 * (from) function and put the original code back */
void reljmp_unregister(struct reljmp *jmp)
{
	DVKDEBUG(DBGLVL0,"reljmp: unregister %s\n", jmp->from_symbol_name);

	reljmp_lock();
	reljmp_text_poke((void *)jmp->from_addr, jmp->from_code,
			     RELATIVEJUMP_SIZE);
	reljmp_unlock();

	DVKDEBUG(DBGLVL0,"reljmp: unpatched %s\n", jmp->from_symbol_name,
			    (void *)jmp->from_addr);
}

/* Modify the first few bytes of the original (from) function and insert a
 * relative jump instruction to the replacement (to) function */
void reljmp_register(struct reljmp *jmp)
{
	DVKDEBUG(DBGLVL0,"reljmp: register %s\n", jmp->from_symbol_name);
	
	reljmp_lock();
	reljmp_text_poke((void *)jmp->from_addr, jmp->jump_code,
			     RELATIVEJUMP_SIZE);
	reljmp_unlock();

	DVKDEBUG(DBGLVL0,"reljmp: patched ", jmp->from_symbol_name,
			    (void *)jmp->from_addr);
}

/* Look up the addresses of required functions and symbols that are not
 * exported to modules */
int reljmp_init_once(void)
{
	printk("reljmp: initialize\n");

	RELJMP_LOOKUP_TYPE(jump_label_mutex, (struct mutex *));
	RELJMP_LOOKUP_TYPE(text_mutex, (struct mutex *));
	RELJMP_LOOKUP_FUNC(text_poke);

	return 0;
}

/* Initialize the jump and fill in the missing pieces of the jump structure */
int reljmp_init(struct reljmp *jmp)
{
	int retval;

	printk("reljmp: initialize %s\n", jmp->from_symbol_name);

	/* Lookup the original (from) function and save the first few bytes so
	 * that we can restore it again */
	jmp->from_addr = kallsyms_lookup_name(jmp->from_symbol_name);
	if (!jmp->from_addr) {
		printk(KERN_ERR "reljmp: failed to lookup %s\n",
		       jmp->from_symbol_name);
		return -ENODEV;
	}
	memcpy(jmp->from_code, (void *)jmp->from_addr, RELATIVEJUMP_SIZE);
	reljmp_info("reljmp: original ", jmp->from_symbol_name,
		    (void *)jmp->from_addr);

	/* Lookup the replacement (to) function */
	jmp->to_addr = kallsyms_lookup_name(jmp->to_symbol_name);
	if (!jmp->to_addr) {
		printk(KERN_ERR "reljmp: failed to lookup %s\n",
		       jmp->to_symbol_name);
		return -ENODEV;
	}
	reljmp_info("reljmp: replacement ", jmp->to_symbol_name,
		    (void *)jmp->to_addr);

	/* Calculate the relative jump address and assemble the jump code */
	jmp->relative_addr = (s32)((long)(jmp->to_addr) -
				   ((long)(jmp->from_addr) + 5));
	jmp->jump_code[0] = RELATIVEJUMP_OPCODE;
	*(s32 *)&jmp->jump_code[1] = jmp->relative_addr;
	reljmp_info("reljmp: jump code", "", (void *)jmp->jump_code);

	/* Call the init handler */
	if (jmp->init_handler) {
		retval = jmp->init_handler();
		if (retval) {
			printk(KERN_ERR "reljmp: failed to run %s "
			       "init_handler\n", jmp->from_symbol_name);
			return retval;
		}
	}

	return 0;
}
