PYTHONBIN=$HOME/miniconda3/envs/dcs/bin/python

source ~/.bash_profile
conda activate dcs

cd code/DetCtrl

case "$1" in
	gui)
	    ($PYTHONBIN DC_gui.py)
            ;;

	cli)
	    ($PYTHONBIN DC_cli.py)
            ;;
         
        *)
            echo $"Usage: $0 {gui|cli}"
            exit 1
esac


