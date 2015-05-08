/*
 * manager.c
 *
 *  Created on: Nov 20, 2013
 *      Author: divyanka
 */

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "project3.h"
#include "router.c"

FILE *lookuptable, *manager;
void create_connectivity_table_with_ports(int);
void *thread_call(void *);
int counter;
pthread_t thread;
struct manager_file man_file;
struct udp_table udp_port_details;
struct connectivity_table con_table;
struct data_packet_forwarding data_packet;
char *buffer_global;
int no_of_threads, no_of_threads_1, no_of_threads_2, no_of_threads_3;
int flag =0;
int lsp_counter, packet_counter;
int portno=4493;

void get_struct_from_file()
{
	printf("getting Details from file\n");
	int i;
	char *buffer;
	char **details;

	/*Get Number of Routers*/

	buffer = (char*) malloc(50 * sizeof(char *));
	fgets(buffer, 50, lookuptable);
	man_file.number_of_nodes= atoi(buffer);

	/*Get Router connection details*/

	int k=0;
	char c;
	while ((c=fgetc(lookuptable))!= EOF)
	{
		if(c == '\n')
		{
			k++;
		}
	}
	k++;
	man_file.number_of_lines_in_file = k;
	details = (char**) malloc(sizeof(char*) * k);
	for (i = 0; i < k; i++)
	{
		details[i] = (char*) malloc(sizeof(char) * 50);
	}
	fseek(lookuptable , 0, SEEK_SET);
	i=0;
	for(i=0; i< k; i++)
	{
		fgets(details[i], 50, lookuptable);
	}
	i =0;
	while(1)
	{
		lsp_counter++;
		if(strcmp(details[i], "-1\n")==0)
		{
			break;
		}
		i++;
	}
	i++;
	while(1)
	{
		packet_counter++;
		if(strcmp(details[i], "-1\n")==0)
		{
			break;
		}
		i++;
	}
	char **lsp_temp, **packet_temp;
	lsp_temp = (char**) malloc(sizeof(char*) * lsp_counter);
	for (i = 0; i < lsp_counter; i++)
	{
		lsp_temp[i] = (char*) malloc(sizeof(char) * 50);
	}
	packet_temp = (char**) malloc(sizeof(char*) * packet_counter);
	for (i = 0; i < packet_counter; i++)
	{
		packet_temp[i] = (char*) malloc(sizeof(char) * 50);
	}
	for (i = 0; i < lsp_counter; i++)
	{
		bzero(lsp_temp[i], strlen(lsp_temp[i]));
	}
	for (i = 0; i < packet_counter; i++)
	{
		bzero(packet_temp[i], strlen(packet_temp[i]));
	}
	i=0;
	while(1)
	{
		strcpy(lsp_temp[i], details[i]);
		if(strcmp(details[i], "-1\n")==0)
		{
			break;
		}
		i++;
	}
	i++;
	int j=0;
	while(1)
	{
		strcpy(packet_temp[j], details[i]);
		if(strcmp(details[i], "-1\n")==0)
		{
			break;
		}
		i++;
		j++;
	}

	man_file.first_entry = (char**) malloc(sizeof(char*) * lsp_counter-2);
	for (i = 0; i < lsp_counter-2; i++)
	{
		man_file.first_entry[i] = (char*) malloc(sizeof(char) * 50);
	}
	man_file.second_entry = (char**) malloc(sizeof(char*) * lsp_counter-2);
	for (i = 0; i < lsp_counter-2; i++)
	{
		man_file.second_entry[i] = (char*) malloc(sizeof(char) * 50);
	}
	man_file.cost = (char**) malloc(sizeof(char*) * lsp_counter-2);
	for (i = 0; i < lsp_counter-2; i++)
	{
		man_file.cost[i] = (char*) malloc(sizeof(char) * 50);
	}
	char **buffer_cost;
	buffer_cost = (char**) malloc(sizeof(char*) * lsp_counter-2);
	for (i = 0; i < lsp_counter-2; i++)
	{
		buffer_cost[i] = (char*) malloc(sizeof(char) * 50);
	}
	data_packet.from = (char**) malloc(sizeof(char*) * packet_counter-1);
	for (i = 0; i < packet_counter-1; i++)
	{
		data_packet.from[i] = (char*) malloc(sizeof(char) * 50);
	}
	data_packet.to = (char**) malloc(sizeof(char*) * packet_counter-1);
	for (i = 0; i < packet_counter-1; i++)
	{
		data_packet.to[i] = (char*) malloc(sizeof(char) * 50);
	}
	for(i=1; i < lsp_counter-1; i++)
	{
		man_file.first_entry[i-1] = strtok(lsp_temp[i], " ");
		man_file.second_entry[i-1] = strtok(NULL, " ");
		buffer_cost[i-1] = strtok(NULL, " ");
		man_file.cost[i-1] = strtok(buffer_cost[i-1], "\n");
	}
	for(i=0; i < packet_counter-1; i++)
	{
		data_packet.from[i] = strtok(packet_temp[i], " ");
		data_packet.to[i] = strtok(NULL, "\n");
	}
	fclose(lookuptable);
}

void parent()
{
	int a;
	sleep(5);
	int status = 0;
	for(a=0;a<man_file.number_of_nodes;a++)
	{
		wait(&status);
	}
	printf("Goodbye from Parent function!\n");
	exit(0);
}

void *TCP_server_manager(void *socka)
{
	printf("In TCP Server!\n");

	int sockfd, newsockfd;
	int *newsockfd_copy;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	sockfd = *(int *)socka;
	if (sockfd < 0)
		printf("ERROR opening socket\n");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	time_stamp(manager);
	fprintf(manager, "Socket created\n");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("ERROR on binding\n");
	}
	time_stamp(manager);
	fprintf(manager, "Socket Binded\n");
	while(1)
	{
		listen(sockfd,5);
		time_stamp(manager);
		fprintf(manager, "TCP Server Listening at port number %d\n", portno);
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			printf("ERROR on accept\n");

		newsockfd_copy = (int *) malloc(sizeof(int *) * 1);
		*newsockfd_copy = newsockfd;
		pthread_create(&thread, NULL, thread_call, (void*)newsockfd_copy);
	}
	close(newsockfd);
	close(sockfd);
	return(0);
}

void *thread_call(void *sock)
{
	int sock1, read1, node_id;
	sock1 = *(int *)sock;
	char *buff;
	buff = (char*)malloc(sizeof(char*)*5000);
	read1 = read(sock1, buff, 500);
	if(read1 <0)	 //port number read
	{
		printf("read error\n");
	}
	//assigning port numbers to node numbers

	sprintf(udp_port_details.node_num[no_of_threads], "%d", no_of_threads);
	strcpy(udp_port_details.assigned_port[no_of_threads], buff);
	node_id = no_of_threads;
	time_stamp(manager);
	fprintf(manager, "received message at node number %s is: %s\n", udp_port_details.node_num[no_of_threads], buff);


	no_of_threads++;
	//only if all port nos are received should the program proceed
	while(1)
	{
		if(no_of_threads < man_file.number_of_nodes)
		{
			continue;
		}
		else
		{
			break;
		}
	}
	int k;
	time_stamp(manager);
	printf("Node Number %s has port number %s\n", udp_port_details.node_num[node_id], udp_port_details.assigned_port[node_id]);
	fprintf(manager, "Node Number %s has port number %s\n", udp_port_details.node_num[node_id], udp_port_details.assigned_port[node_id]);
	//create connectivity table along with port numbers
	printf("creating connectivity table!!\n");
	if(node_id == 0)
	{
		create_connectivity_table_with_ports(node_id);      //Create connectivity table
	}
	while(1)
	{
		if(flag==0)
		{
			continue;
		}
		else
		{
			break;
		}
	}
	if(write(sock1, con_table.neighbors_cost[node_id], 500)<0)   //connectivity table send
	{
		printf("Error in writing to TCP client\n");
	}
	printf("Connectivity table %s sent to router number %d\n",con_table.neighbors_cost[node_id], node_id);
	time_stamp(manager);
	fprintf(manager, "Connectivity table %s sent to router number %d\n",con_table.neighbors_cost[node_id], node_id);

	char *ready_recv;
	ready_recv = (char*)malloc(sizeof(char*)*50);
	if(read(sock1, ready_recv, 50) <0)	 //Ready read
	{
		printf("read error\n");
	}
	time_stamp(manager);
	fprintf(manager, "received message at node number %s is: %s\n", udp_port_details.node_num[no_of_threads_1], ready_recv);

	no_of_threads_1++;
	while(1)
	{
		if(no_of_threads_1 < man_file.number_of_nodes)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	int write1;
	write1=write(sock1, "READY OK", 10);
	if(write1<0)    	 //READY OK send
	{
		printf("Error in writing to TCP client\n");
	}
	time_stamp(manager);
	fprintf(manager,"READY OK sent to router number %d\n", node_id);
	printf("READY OK sent to router number %d\n", node_id);
	bzero(buff, strlen(buff));

	read1 = read(sock1, buff, 500);
	if(read1 <0)
	{
		printf("read error\n");
	}
	time_stamp(manager);
	fprintf(manager, "received message at node number %s is: %s\n", udp_port_details.node_num[no_of_threads_2], buff);
	if(strcmp(buff, "Link_establishment_done") == 0)
	{
		no_of_threads_2++;
		while(1)
		{
			if(no_of_threads_2 < man_file.number_of_nodes)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		write1 = write(sock1, "Link_Establish_Done_ACK", strlen("Link_Establish_Done_ACK"));
		if(write1<0)
		{
			printf("Error in writing\n");
		}
		time_stamp(manager);
		fprintf(manager, "Link_Establish_Done_ACK sent to node number: %d\n", node_id);
		printf("Link_Establish_Done_ACK sent to node number: %d\n", node_id);
	}
	printf("Broadcasting...\n");

	bzero(buff, strlen(buff));
	read1 = read(sock1, buff, 500);
	if(read1 <0)
	{
		printf("read error\n");
	}
	time_stamp(manager);
	fprintf(manager, "received message at node number %s is: %s\n", udp_port_details.node_num[no_of_threads_3], buff);
	if(strcmp(buff, "Forwarding table created") == 0)
	{
		no_of_threads_3++;
		while(1)
		{
			if(no_of_threads_3 < man_file.number_of_nodes)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		write1 = write(sock1, "Forwarding table created for all nodes", strlen("Forwarding table created for all nodes"));
		if(write1<0)
		{
			printf("Error in writing\n");
		}
		time_stamp(manager);
		fprintf(manager, "Forwarding table created for all nodes sent to node number: %d\n", node_id);
	}
	bzero(buff, strlen(buff));
	read1 = read(sock1, buff, 500);
	if(read1 <0)
	{
		printf("read error\n");
	}
	if(strcmp(buff, "start sending data packet details") == 0)
	{
		no_of_threads_3++;
		while(1)
		{
			if(no_of_threads_3 < (2*man_file.number_of_nodes))
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
	int i;
	char *send_data;
	send_data = (char *)malloc(sizeof(char *)*100);
	for(i=0; i<packet_counter-1; i++)
	{
		if(atoi(data_packet.from[i]) == node_id)
		{
			strcpy(send_data, "send_to:");
			strcat(send_data, data_packet.to[i]);
			strcat(send_data, "-");
			strcat(send_data, "Hello");
			strcat(send_data, ":");
			write1 = write(sock1, send_data, strlen(send_data));
			if(write1<0)
			{
				printf("Error in writing\n");
			}
			time_stamp(manager);
			fprintf(manager, "%s sent to node number: %d\n", send_data, node_id);
			printf("In manager sending %s to tcp client number: %d\n", send_data, node_id);
			bzero(send_data, strlen(send_data));
		}
		sleep(5);
	}
	write1 = write(sock1, "exit:d-", strlen("exit:d-"));
	if(write1<0)
	{
		printf("Error in writing\n");
	}
	time_stamp(manager);
	//printf("In manager sending exit:d- to %d\n", node_id);
	fprintf(manager, "exit sent to node number: %d\n", node_id);

	return 0;
}

void create_connectivity_table_with_ports(int node_id)
{
	int i = 0;
	int line_counter=0;
	int s = 0;
	do
	{
		line_counter = 0;
		char s_char[2];
		sprintf(s_char, "%d", s);
		for(i=0; i<lsp_counter-2; i++)
		{
			if(strcmp(man_file.first_entry[i], s_char) == 0)
			{
				strcat(con_table.neighbors_cost[s], man_file.first_entry[i]);
				strcat(con_table.neighbors_cost[s], "-");
				strcat(con_table.neighbors_cost[s], man_file.second_entry[i]);
				strcat(con_table.neighbors_cost[s], "$");
				strcat(con_table.neighbors_cost[s], man_file.cost[i]);
				strcat(con_table.neighbors_cost[s], "p");
				strcat(con_table.neighbors_cost[s], udp_port_details.assigned_port[atoi(man_file.second_entry[i])]);
				strcat(con_table.neighbors_cost[s], "\n");
				line_counter++;
			}
			else if(strcmp(man_file.second_entry[i],s_char) == 0)
			{
				strcat(con_table.neighbors_cost[s], man_file.second_entry[i]);
				strcat(con_table.neighbors_cost[s], "-");
				strcat(con_table.neighbors_cost[s], man_file.first_entry[i]);
				strcat(con_table.neighbors_cost[s], "$");
				strcat(con_table.neighbors_cost[s], man_file.cost[i]);
				strcat(con_table.neighbors_cost[s], "p");
				strcat(con_table.neighbors_cost[s], udp_port_details.assigned_port[atoi(man_file.first_entry[i])]);
				strcat(con_table.neighbors_cost[s], "\n");
				line_counter++;
			}
		}
		s++;
	}while(s < man_file.number_of_nodes);
	flag=1;
}

int main(int argc, char *argv[])
{
	printf("In main Function\n");
	if(argc <2)
	{
		printf("Please enter file name as argument\n");
		exit(0);
	}
	lookuptable= fopen(argv[1], "r");
	manager = fopen("manager.out", "w");
	counter = 0;
	no_of_threads = 0;
	no_of_threads_1 = 0;
	no_of_threads_2 = 0;
	no_of_threads_3 = 0;
	pid_t childpid;
	lsp_counter = 0;
	packet_counter = 0;
	man_file.no_of_children = 0;
	get_struct_from_file();
	int i;
	udp_port_details.node_num = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		udp_port_details.node_num[i] = (char*) malloc(sizeof(char *) * 5);

	}
	udp_port_details.assigned_port = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		udp_port_details.assigned_port[i] = (char*) malloc(sizeof(char *) * 10);
	}
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		bzero(udp_port_details.node_num[i], 5);
		bzero(udp_port_details.assigned_port[i],10);
	}
	con_table.neighbors_cost = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		con_table.neighbors_cost[i] = (char*) malloc(sizeof(char) * 500);
	}
	con_table.number_of_lines_in_string = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		con_table.number_of_lines_in_string[i] = (char*) malloc(sizeof(char) * 500);
	}
	LSP_details.node_num = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		LSP_details.node_num[i] = (char*) malloc(sizeof(char *) * 5);

	}
	LSP_details.assigned_port = (char**) malloc(sizeof(char*) * man_file.number_of_nodes);
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		LSP_details.assigned_port[i] = (char*) malloc(sizeof(char *) * 10);
	}
	for (i = 0; i < man_file.number_of_nodes; i++)
	{
		bzero(LSP_details.node_num[i], 5);
		bzero(LSP_details.assigned_port[i],10);
	}
	int sock1, *sock;
	buffer_global = (char *)malloc(sizeof(char*)*500);
	sock1 = socket(AF_INET, SOCK_STREAM, 0);
	if(sock1 < 0)
	{
		printf("Error in creating socket");
	}
	printf("TCP Socket Created!\n");
	sock = (int *)malloc(sizeof(int *) * 1);
	*sock = sock1;
	pthread_create(&thread, NULL, TCP_server_manager, (void*)sock);
	printf("Thread created and sleeping for 5 seconds\n");
	sleep(5);
	for(i=0;i< man_file.number_of_nodes; i++)
	{
		childpid = fork();
		if(childpid < 0)
		{
			printf("Fork process failed!\n");
		}
		else if (childpid == 0)
		{
			printf("Child %d created inside for loop\n", i);
			time_stamp(manager);
			fprintf(manager, "Child %d created inside for loop\n", i);
			TCP_client(i);
			exit(0);
		}
	}
	printf("All children have exited and waiting for parent to exit\n");
	sleep(5);
	parent();
	return(0);
}
