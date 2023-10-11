# Installation for dcs 

1. Download `dcs_pack` directory from GitHub

2. Unzip the compressed file `../dcs_pack/installation/macie_v5.3_centos.tar.gz` in the same folder

3. Open and edit 
	- In "../dcs_pack/installation/dc-core.service"
	```
	ExecStart=your home directory/dcs_pack/installation/run_dc_core.sh
	```
	- In "../dcs_pack/installation/run_dc_core.sh"
	```
	HOME=your home directory
	```

4. Setup the macie library and python library
	```
	$ cd $HOME/dcs_pack/installation	
	$ sh dcs_setup.sh
	```
	- If some error is related to `sudo` command, edit `/etc/sudoers`:
	
	`# visudo /etc/sudoers`
	```
	## Allow root to run any commands anywhere
	root    ALL=(ALL)       ALL
	"your dcs name"    ALL=(ALL)       ALL
	## Allows people in group wheel to run all commands
	%wheel  ALL=(ALL)       ALL
	"your dcs name"    ALL=(ALL)       ALL
	## Same thing without a password
	# %wheel        ALL=(ALL)       NOPASSWD: ALL
	"your dcs name"            ALL=(ALL)       NOPASSWD: ALL
	```
	(if read-only error :wq -> :w!)
	
	After setup finished, exit the current terminal, **open new terminal**!!!
5. Set nfs mount
	
	- dcs: server / ics, TelOps: client
	```
	# dnf install nfs-utils
	# systemctl start nfs-server.service
	# systemctl enable nfs-server.service
	# systemctl status nfs-server.service
	```
	`# vi /etc/exports`
	```
	$HOME/DCS/Data 192.168.1.203(rw,sync,no_root_squash)
	$HOME/DCS/Data "ip address of TelOps"(rw,sync,no_root_squash)
	```
	`# exportfs -arv`
	```
	exporting 192.168.1.203:$HOME/DCS/Data
	exporting "ip address of TelOps":$HOME/DCS/Data
	```
	`# exportfs -s`
	```
	$HOME/DCS/Data  192.168.1.203(sync,wdelay,hide,no_subtree_check,sec=sys,rw,secure,no_root_squash,no_all_squash)
	$HOME/DCS/Data  "ip address of TelOps"(sync,wdelay,hide,no_subtree_check,sec=sys,rw,secure,no_root_squash,no_all_squash)
	```
	```
	# firewall-cmd --permanent --add-service=nfs
	# firewall-cmd --permanent --add-service=rpc-bind
	# firewall-cmd --permanent --add-service=mountd
	# firewall-cmd --reload
	```
6. Install rabbitmq server for local (between DC gui and DC core)
	```
	$ yum install -y epel-release
	$ yum install -y erlang
	```
	- yum install -y rabbitmq-server
	```
	$ wget https://github.com/rabbitmq/rabbitmq-server/releases/download/v3.8.9/rabbitmq-server-3.8.9-1.el7.noarch.rpm
	$ sudo rpm --import https://www.rabbitmq.com/rabbitmq-signing-key-public.asc
	$ sudo yum install -y rabbitmq-server-3.8.9-1.el7.noarch.rpm
	```
	```
	# rabbitmq-plugins enable rabbitmq_management
	# systemctl list-unit-files | grep rabbitmq-server
	# systemctl enable rabbitmq-server
	# systemctl start rabbitmq-server
	# rabbitmqctl list_users
	# rabbitmqctl add_user "your dcs name" kasi2023
	# rabbitmqctl set_user_tags "your dcs name" administrator
	# rabbitmqctl list_permissions
	# rabbitmqctl delete_user test
	# rabbitmqctl set_permissions -p / "your dcs name" ".*" ".*" ".*"
	```
	- For reset queue (deleted users)
	```
	# rabbitmqctl stop_app
	# rabbitmqctl reset
	# rabbitmqctl start_app
	```
	- For firewall
	```
	# firewall-cmd --permanent --zone=public --add-port=5672/tcp
	# firewall-cmd --reload
	```
	- For registering service
	```
	# systemctl enable rabbitmq-server
	```
	- For starting service
	```
	# systemctl start rabbitmq-server
	```
	- For open tcp port!!
	```
	# firewall-cmd --permanent --zone=public --add-port=5672/tcp
	# firewall-cmd --reload
	```
7. Start software
	```
	$ sudo systemctl daemon-reload
	$ sudo systemctl enable dc-core.service
	$ sudo systemctl start dc-core.service
	$ sudo systemctl status dc-core.service
	```
	- If some failure, 
   	```
	$ sudo systemctl stop dc-core.service
	$ sudo systemctl reset-failed
	```
	or
	```
	$ setenforce 0
	```
	After checking permission
	```
	$ cd .../dcs_pack/installation/run_dc_core.sh
	$ ls -lh
	$ sudo chmod 744 run_dc_core.sh
	```
	- If you want to stop,
   	```
	$ sudo systemctl stop dc-core.service
	```
	- If you want to use gui,
	```
	$ sh $HOME/dcs_pack/run_dcs.sh gui
	```
8. For starting "dc-core.service" without error automatically when system is rebooting,
	
	`# vi /etc/selinux/config`
	```
	SELINUX = disabled
	```
