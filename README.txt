Welcome to Connor and Greg's COMP 8005 Project, The IP Port Forwarder, or IPPF as it is labelled in the accompanying programs. It directs all traffic on one port, to a specific IP:Port pair.

How To Setup:
	- Modify the "IPP_Pairs.conf" located within the "Code" folder using the following structure:
		"LISTENPORT-FORWARDIP:FORWARDPORT"

	- The above is an example of how to setup a list of IP and Port pairs, citing a listen port, with an IP:Port pair to direct that traffic to.

	- Build the file using the following gcc commands "gcc -Wall IPPF.c -o IPPF -lpthread"

How to Use:
	- Run the gcc'd "IPPF" file in the same folder as the IPP_Pairs.conf file. This will run the port forwarder program and redirect the indicated traffic.

Notes and Troubleshooting:
	- Each line of the "IPP_Pairs.conf" file creates a thread, which then forks into processes to forward data. 
	- The "IPP_Pairs.conf" file follows the exact syntax shown above, example ->      80-192.168.0.14:80
