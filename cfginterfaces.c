/*
 * This is automatically generated callbacks file
 * It contains 3 parts: Configuration callbacks, RPC callbacks and state data callbacks.
 * Do NOT alter function signatures or any structures unless you know exactly what you are doing.
 */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libnetconf_xml.h>

#include "cfginterfaces.h"
#include "config.h"

/* transAPI version which must be compatible with libnetconf */
int transapi_version = 6;

/* Signal to libnetconf that configuration data were modified by any callback.
 * 0 - data not modified
 * 1 - data have been modified
 */
int config_modified = 0;

/*
 * Determines the callbacks order.
 * Set this variable before compilation and DO NOT modify it in runtime.
 * TRANSAPI_CLBCKS_LEAF_TO_ROOT (default)
 * TRANSAPI_CLBCKS_ROOT_TO_LEAF
 */
const TRANSAPI_CLBCKS_ORDER_TYPE callbacks_order = TRANSAPI_CLBCKS_ROOT_TO_LEAF;

/* Do not modify or set! This variable is set by libnetconf to announce edit-config's error-option
Feel free to use it to distinguish module behavior for different error-option values.
 * Possible values:
 * NC_EDIT_ERROPT_STOP - Following callback after failure are not executed, all successful callbacks executed till
                         failure point must be applied to the device.
 * NC_EDIT_ERROPT_CONT - Failed callbacks are skipped, but all callbacks needed to apply configuration changes are executed
 * NC_EDIT_ERROPT_ROLLBACK - After failure, following callbacks are not executed, but previous successful callbacks are
                         executed again with previous configuration data to roll it back.
 */
NC_EDIT_ERROPT_TYPE erropt = NC_EDIT_ERROPT_NOTSET;

/* always learnt in the first callback, used by every other */
static char* iface_name = NULL;


/* flag to indicate ipv4-enabled change - we have to reapply the configured addresses -
 * - ignore any changes as they would be applied twice */


static int finish(char* msg, int ret, struct nc_err** error) {
	if (ret != EXIT_SUCCESS && error != NULL) {
		*error = nc_err_new(NC_ERR_OP_FAILED);
		if (msg != NULL) {
			nc_err_set(*error, NC_ERR_PARAM_MSG, msg);
		}
	}

	if (msg != NULL) {
		nc_verb_error(msg);
		free(msg);
	}

	return ret;
}

xmlNodePtr parse_iface_config(const char* if_name, xmlNsPtr ns, char** msg) {


	xmlNodePtr interface;


	interface = xmlNewNode(ns, BAD_CAST "interface");
	xmlNewTextChild(interface, interface->ns, BAD_CAST "name", BAD_CAST if_name);


	return interface;

}

/**
 * @brief Initialize plugin after loaded and before any other functions are called.

 * This function should not apply any configuration data to the controlled device. If no
 * running is returned (it stays *NULL), complete startup configuration is consequently
 * applied via module callbacks. When a running configuration is returned, libnetconf
 * then applies (via module's callbacks) only the startup configuration data that
 * differ from the returned running configuration data.

 * Please note, that copying startup data to the running is performed only after the
 * libnetconf's system-wide close - see nc_close() function documentation for more
 * information.

 * @param[out] running	Current configuration of managed device.

 * @return EXIT_SUCCESS or EXIT_FAILURE
 */

char** iface_get_ifcs(unsigned char config, unsigned int* dev_count, char** msg) {
	DIR* dir;
	struct dirent* dent;
	char** ret = NULL;
#if defined(REDHAT) || defined(SUSE)
	char* path, *variable, *suffix = NULL;
	unsigned char normalized;
#endif

	if ((dir = opendir("/sys/class/net")) == NULL) {
		asprintf(msg, "%s: failed to open \"/sys/class/net\" (%s).", __func__, strerror(errno));
		return NULL;
	}

	while ((dent = readdir(dir))) {
		if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
			continue;
		}

		/* check if the device is managed by ifup/down scripts */
		if (config) {
#if defined(REDHAT) || defined(SUSE)
			asprintf(&path, "%s/ifcfg-%s", IFCFG_FILES_PATH, dent->d_name);
			if (access(path, F_OK) == -1 && errno == ENOENT) {
				free(path);
				continue;
			}
			free(path);

			/* "normalize" the ifcfg file */
			normalized = 1;
			if ((value = read_ifcfg_var(dent->d_name, "IPADDR", NULL)) != NULL) {
				if (remove_ifcfg_var(dent->d_name, "IPADDR", value, NULL) != EXIT_SUCCESS ||
						write_ifcfg_var(dent->d_name, "IPADDRx", value, &suffix) != EXIT_SUCCESS) {
					normalized = 0;
				}
				free(value);

				if (suffix != NULL && (value = read_ifcfg_var(dent->d_name, "PREFIX", NULL)) != NULL) {
					asprintf(&variable, "PREFIX%s", suffix);
					if (remove_ifcfg_var(dent->d_name, "PREFIX", value, NULL) != EXIT_SUCCESS ||
							write_ifcfg_var(dent->d_name, variable, value, NULL) != EXIT_SUCCESS) {
						normalized = 0;
					}
					free(value);
					free(variable);
				}

				if (suffix != NULL && (value = read_ifcfg_var(dent->d_name, "NETMASK", NULL)) != NULL) {
					asprintf(&variable, "NETMASK%s", suffix);
					if (remove_ifcfg_var(dent->d_name, "NETMASK", value, NULL) != EXIT_SUCCESS ||
							write_ifcfg_var(dent->d_name, variable, value, NULL) != EXIT_SUCCESS) {
						normalized = 0;
					}
					free(value);
					free(variable);
				}
				free(suffix);
				suffix = NULL;
			}

			if (!normalized) {
				nc_verb_warning("%s: failed to normalize ifcfg-%s, some configuration problems may occur.", __func__, dent->d_name);
			}
#endif
		}

		/* add a device */
		if (ret == NULL) {
			*dev_count = 1;
			ret = malloc(sizeof(char*));
		} else {
			++(*dev_count);
			ret = realloc(ret, (*dev_count)*sizeof(char*));
		}
		ret[*dev_count-1] = strdup(dent->d_name);
	}
	closedir(dir);

	if (ret == NULL) {
		asprintf(msg, "%s: no %snetwork interfaces detected.", __func__, (config ? "managed " : ""));
	}

	return ret;
}

int transapi_init(xmlDocPtr * running)
{
	int i;
	unsigned int dev_count;
	xmlNodePtr root, interface;
	xmlNsPtr ns;
	char** devices, *msg = NULL;
#if defined(AVAHI_DAEMON) || defined(AVAHI_AUTOIPD)
	FILE* output;
	char* line = NULL, *cmd;
	size_t len;
#endif

	devices = iface_get_ifcs(1, &dev_count, &msg);
	if (devices == NULL) {
		return finish(msg, EXIT_FAILURE, NULL);
	}

	/* kill avahi SW interfering with our IPv4 configuration */
#ifdef AVAHI_DAEMON
	asprintf(&cmd, "avahi-daemon --kill 2>&1");
	output = popen(cmd, "r");
	free(cmd);

	if (output == NULL) {
		nc_verb_error("Failed to execute command in %s.", __func__);
		return EXIT_FAILURE;
	}

	if (getline(&line, &len, output) != -1 && strstr(line, "No such file or directory") == NULL) {
		nc_verb_error("%s: %s", __func__, line);
		free(line);
		pclose(output);
		return EXIT_FAILURE;
	}

	free(line);
	line = NULL;
	pclose(output);
#endif

#ifdef AVAHI_AUTOIPD
	for (i = 0; i < dev_count; ++i) {
		asprintf(&cmd, "avahi-autoipd --kill %s 2>&1", devices[i]);
		output = popen(cmd, "r");
		free(cmd);

		if (output == NULL) {
			nc_verb_error("Failed to execute command in %s.", __func__);
			return EXIT_FAILURE;
		}

		if (getline(&line, &len, output) != -1 && strstr(line, "No such file or directory") == NULL) {
			nc_verb_error("%s: interface %s fail: %s", __func__, devices[i], line);
			free(line);
			pclose(output);
			return EXIT_FAILURE;
		}

		free(line);
		line = NULL;
		pclose(output);
	}
#endif

	*running = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL, BAD_CAST "interfaces");
	ns = xmlNewNs(root, BAD_CAST "urn:ietf:params:xml:ns:yang:ietf-interfaces", NULL);
	xmlSetNs(root, ns);

	xmlDocSetRootElement(*running, root);

	/* Go through the array and process all devices */
	for (i = 0; i < dev_count; i++) {
		interface = parse_iface_config(devices[i], ns, &msg);
		free(devices[i]);

		if (interface == NULL) {
			if (msg != NULL) {
				nc_verb_error(msg);
				free(msg);
				msg = NULL;
			}
		}

		xmlAddChild(root, interface);
	}

	free(devices);

	return EXIT_SUCCESS;
}

/**
 * @brief Free all resources allocated on plugin runtime and prepare plugin for removal.
 */
void transapi_close(void)
{
	free(iface_name);
}

/**
 * @brief Retrieve state data from device and return them as XML document
 *
 * @param model	Device data model. libxml2 xmlDocPtr.
 * @param running	Running datastore content. libxml2 xmlDocPtr.
 * @param[out] err  Double pointer to error structure. Fill error when some occurs.
 * @return State data as libxml2 xmlDocPtr or NULL in case of error.
 */
xmlDocPtr get_state_data (xmlDocPtr model, xmlDocPtr running, struct nc_err **err)
{
	int i;
	unsigned int dev_count;
	xmlDocPtr doc;
	xmlNodePtr root, interface;
	xmlNsPtr ns;
	char** devices, *msg = NULL;


	devices = iface_get_ifcs(0, &dev_count, &msg);
	if (devices == NULL) {
		finish(msg, 0, err);
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL, BAD_CAST "interfaces-state");
	ns = xmlNewNs(root, BAD_CAST "urn:ietf:params:xml:ns:yang:ietf-interfaces", NULL);
	xmlSetNs(root, ns);

	xmlDocSetRootElement(doc, root);

	/* Go through the array and process all devices */
	for (i = 0; i < dev_count; i++) {
		interface = xmlNewChild(root, root->ns, BAD_CAST "interface", NULL);
		xmlNewTextChild(interface, interface->ns, BAD_CAST "name", BAD_CAST devices[i]);

		if (msg != NULL) {
			nc_verb_error(msg);
			free(msg);
			msg = NULL;
		}
		free(devices[i]);
	}

	free(devices);

	return doc;
}
/*
 * Mapping prefixes with namespaces.
 * Do NOT modify this structure!
 */
struct ns_pair namespace_mapping[] = {
	{"if", "urn:ietf:params:xml:ns:yang:ietf-interfaces"},
	{"preempt", "urn:ieee:std:802.1Q:yang:ieee802-dot1q-sched"},
	{"sched", "urn:ieee:std:802.1Q:yang:ieee802-dot1q-preemption"},
	{NULL, NULL}
};

/*
* CONFIGURATION callbacks
* Here follows set of callback functions run every time some change in associated part of running datastore occurs.
* You can safely modify the bodies of all function as well as add new functions for better lucidity of code.
*/

/**
 * @brief This callback will be run when node in path /if:interfaces/if:interface changes
 *
 * @param[in] data	Double pointer to void. Its passed to every callback. You can share data using it.
 * @param[in] op	Observed change in path. XMLDIFF_OP type.
 * @param[in] node	Modified node. if op == XMLDIFF_REM its copy of node removed.
 * @param[out] error	If callback fails, it can return libnetconf error structure with a failure description.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
/* !DO NOT ALTER FUNCTION SIGNATURE! */
int callback_if_interfaces_if_interface (void ** data, XMLDIFF_OP op, xmlNodePtr old_node, xmlNodePtr new_node, struct nc_err** error)
{
	char* msg;
	xmlNodePtr cur;

	free(iface_name);
	iface_name = NULL;

	nc_verb_verbose("%s is calle", __func__);


	for (cur = ((op & XMLDIFF_REM) ? old_node->children : new_node->children); cur != NULL; cur = cur->next) {
		if (cur->children == NULL || cur->children->content == NULL) {
			continue;
		}

		if (xmlStrEqual(cur->name, BAD_CAST "name")) {
			iface_name = strdup((char*)cur->children->content);
		}
	}

	if (iface_name == NULL) {
		msg = strdup("Could not retrieve the name of an interface.");
		return finish(msg, EXIT_FAILURE, error);
	}

	return EXIT_SUCCESS;
}

/**
 * @brief This callback will be run when node in path /if:interfaces/if:interface/if:enabled changes
 *
 * @param[in] data	Double pointer to void. Its passed to every callback. You can share data using it.
 * @param[in] op	Observed change in path. XMLDIFF_OP type.
 * @param[in] node	Modified node. if op == XMLDIFF_REM its copy of node removed.
 * @param[out] error	If callback fails, it can return libnetconf error structure with a failure description.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
/* !DO NOT ALTER FUNCTION SIGNATURE! */
int callback_gate_enabled (void ** data, XMLDIFF_OP op, xmlNodePtr old_node, xmlNodePtr new_node, struct nc_err** error) {
	
	char* msg = NULL;
	xmlNodePtr node;


	node = (op & XMLDIFF_REM ? old_node : new_node);

	if (node->children == NULL || node->children->content == NULL) {
		asprintf(&msg, "Empty node in \"%s\", internal error.", __func__);
		return finish(msg, EXIT_FAILURE, error);
	}
	
	nc_verb_verbose("%s is calle", __func__);
	if (node->children->content)
		nc_verb_verbose("enable value is:%s", BAD_CAST node->children->content);

	return EXIT_SUCCESS;
}

/*
* Structure transapi_config_callbacks provide mapping between callback and path in configuration datastore.
* It is used by libnetconf library to decide which callbacks will be run.
* DO NOT alter this structure
*/
struct transapi_data_callbacks clbks =  {
	.callbacks_count = 2,
	.data = NULL,
	.callbacks = {
		{.path = "/if:interfaces/if:interface", .func = callback_if_interfaces_if_interface},
		{.path = "/if:interfaces/if:interface/sched:gate-parameters/sched:gate-enabled", .func = callback_gate_enabled},
	}
};

/*
* RPC callbacks
* Here follows set of callback functions run every time RPC specific for this device arrives.
* You can safely modify the bodies of all function as well as add new functions for better lucidity of code.
* Every function takes array of inputs as an argument. On few first lines they are assigned to named variables. Avoid accessing the array directly.
* If input was not set in RPC message argument in set to NULL.
*/

/*
* Structure transapi_rpc_callbacks provide mapping between callbacks and RPC messages.
* It is used by libnetconf library to decide which callbacks will be run when RPC arrives.
* DO NOT alter this structure
*/
struct transapi_rpc_callbacks rpc_clbks = {
	.callbacks_count = 0,
	.callbacks = {
	}
};

