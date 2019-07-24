#ifndef _CONFIG_H_
#define _CONFIG_H_

#define DEBIAN

/* avahi SW presence */
#define AVAHI_DAEMON
#define AVAHI_AUTOIPD

/* installed DHCP client (this command followed by the interface will be executed) */
#define DHCP_CLIENT_RENEW "dhclient"
#define DHCP_CLIENT_RELEASE "dhclient -r"

/* path to the device statistics file */
#define DEV_STATS_PATH "/proc/net/dev"

/* directory with ifcfg scripts (on Debian a single file) */
#define IFCFG_FILES_PATH "/etc/network/interfaces"

/* directory with additional bash scripts (executed by if(up/down), not used on Debian) */
#define IFCFG_SCRIPTS_PATH ""

/* sysctl script */
#define SYSCTL_CONF_PATH "/etc/sysctl.conf"

#endif /* _CONFIG_H_ */
