/*
 * p3.c
 *
 *  Created on: Nov 28, 2013
 *      Author: DivyankaBose
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

FILE *router;
time_t ltime;
struct manager_file man_file;
struct udp_table udp_port_details;
struct LSP_details LSP_details;
struct after_broadcast_recd after_broadcast;
struct dijkstra dijk;
void *thread_for_UDP(void *);
void dij();
pthread_t thread;
struct udp_details_after_rec_con_table udp_details_after;
int portnoc = 4493;
int check_forwarding_table(int );
void udp_client(char *, char *);
void udp_client_data(char *, char *);
int UDP_port =8000;

int count_characters(const char *str, char delimiter)
{
	const char *abc = str;
	int count = 0;

	do {
		if (*abc == delimiter)
			count++;
	} while (*(abc++));

	return count;
}

void time_stamp(FILE *fh)
{
	ltime=time(NULL);
	fprintf(fh, "\n%s", asctime( localtime(&ltime) ));
}

//create TCP client

void TCP_client(int p_no)
{
	printf("In TCP Client\n");
	char *ip;
	int port_no;
	port_no = UDP_port+p_no;
	char *port_no_char;
	port_no_char = (char *)malloc(sizeof(char *)* 20);
	sprintf(port_no_char, "%d", port_no);
	ip = (char *)malloc(sizeof(char *) * 15);
	int sockfd, con;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		printf("ERROR opening socket");
	strcpy(ip, "localhost");
	server = gethostbyname(ip);

	if (server == NULL)
	{
		printf("ERROR, host not found\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portnoc);
	con = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

	if (con < 0)
	{
		printf("ERROR connecting");
	}
	printf("Client connected!!\n");

	if(write(sockfd, port_no_char, strlen(port_no_char))<0)      //send port number
	{
		printf("Error in writing port number to manager\n");
	}
	char *recv_data;
	recv_data = (char *)malloc(sizeof(char *)*1000);
	int read1;

	read1 = read(sockfd,recv_data,500);
	if(read1 < 0)
	{
		printf("Error in reading\n");
	}

	char *buffer_con;
	buffer_con = (char *)malloc(sizeof(char *)* 200);
	strcpy(buffer_con, recv_data);
	strtok(buffer_con, "\n");
	strtok(buffer_con, "-");
	char *node_assigned;
	node_assigned = (char *)malloc(sizeof(char *)* 20);
	strcpy(node_assigned, buffer_con);

	// Create router logfile

	char *file_name;
	file_name = (char *)malloc(sizeof(char *)* 30);
	strcpy(file_name, node_assigned);
	strcat(file_name, ".out");
	router = fopen(file_name, "w");

	udp_details_after.self_node_num = atoi(node_assigned);
	udp_details_after.self_port = (char*)malloc(sizeof(char *)*10);
	strcpy(udp_details_after.self_port, port_no_char);
	udp_details_after.self_con_table = (char*)malloc(sizeof(char *)*500);
	strcpy(udp_details_after.self_con_table, recv_data);
	udp_details_after.neighbors = count_characters(recv_data, '\n');

	//get self details

	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");
	time_stamp(router);
	fprintf(router, "Self node is %d\n", udp_details_after.self_node_num);
	fprintf(router, "port number assigned is: %s\n", udp_details_after.self_port);
	fprintf(router, "the connectivity table received from manager: %s\n", udp_details_after.self_con_table);
	fprintf(router, "No of neighbors are: %d\n", udp_details_after.neighbors);
	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");

	int i;
	LSP_details.LSP_details = (char **)malloc(sizeof(char *)* man_file.number_of_nodes);
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		LSP_details.LSP_details[i] = (char *)malloc(sizeof(char *)* 900);
	}
	for(i=0; i<man_file.number_of_nodes; i++)
	{

		bzero(LSP_details.LSP_details[i], strlen(LSP_details.LSP_details[i]));
	}

	LSP_details.Broadcast_details = (char **)malloc(sizeof(char *)* man_file.number_of_nodes);
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		LSP_details.Broadcast_details[i] = (char *)malloc(sizeof(char *)* 900);
	}
	for(i=0; i<man_file.number_of_nodes; i++)
	{

		bzero(LSP_details.Broadcast_details[i], strlen(LSP_details.Broadcast_details[i]));
	}

	char *node_num;
	node_num = (char *)malloc(sizeof(char *)*5);

	char *create_LSP;
	create_LSP = (char *)malloc(sizeof(char *)* 900);
	strcpy(create_LSP, "My_LSP:");
	sprintf(node_num, "%d", udp_details_after.self_node_num);
	strcat(create_LSP, node_num);
	strcat(create_LSP, "c");
	strcat(create_LSP, udp_details_after.self_con_table);
	strcat(create_LSP, "END");									//LSP format: My_LSP:<node_num>c<connectivity table>
	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");
	time_stamp(router);
	fprintf(router, "LSP Created is %s\n", create_LSP);
	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");

	strcpy(LSP_details.LSP_details[udp_details_after.self_node_num], udp_details_after.self_con_table);

	//create UDP socket

	int *p;
	p = (int *)malloc(sizeof(int *)*1);
	pthread_create(&thread, NULL, thread_for_UDP, (void*)p);

	//Get neighbors and their port in an array

	udp_details_after.neighbors_of =(char**) malloc(sizeof(char*) * udp_details_after.neighbors);
	for(i=0; i < udp_details_after.neighbors; i++)
	{
		udp_details_after.neighbors_of[i] = (char*) malloc(sizeof(char) * 50);
	}
	udp_details_after.port_nos_of_nighbors =(char**) malloc(sizeof(char*) * udp_details_after.neighbors);
	for(i=0; i < udp_details_after.neighbors; i++)
	{
		udp_details_after.port_nos_of_nighbors[i] = (char*) malloc(sizeof(char) * 50);
	}

	strcpy(buffer_con, recv_data);
	char **buffer_con1, **buffer_con2, **buffer_con3;
	buffer_con1 = (char **)malloc(sizeof(char *)*udp_details_after.neighbors);
	for(i=0; i < udp_details_after.neighbors; i++)
	{
		buffer_con1[i] = (char*) malloc(sizeof(char) * 50);
	}
	buffer_con2 = (char **)malloc(sizeof(char *)*udp_details_after.neighbors);
	for(i=0; i < udp_details_after.neighbors; i++)
	{
		buffer_con2[i] = (char*) malloc(sizeof(char) * 50);
	}
	buffer_con3 = (char **)malloc(sizeof(char *)*udp_details_after.neighbors);
	for(i=0; i < udp_details_after.neighbors; i++)
	{
		buffer_con3[i] = (char*) malloc(sizeof(char) * 50);
	}
	buffer_con1[0] = strtok(buffer_con, "\n");
	for(i=1; i<udp_details_after.neighbors; i++)
	{
		buffer_con1[i] = strtok(NULL, "\n");
	}
	buffer_con2[0] = strtok(buffer_con1[0], "p");
	udp_details_after.port_nos_of_nighbors[0] = strtok(NULL, "n");
	for(i=1; i<udp_details_after.neighbors; i++)
	{
		buffer_con2[i] = strtok(buffer_con1[i], "p");
		udp_details_after.port_nos_of_nighbors[i] = strtok(NULL, "n");
	}
	buffer_con3[0] = strtok(buffer_con2[0], "-");
	udp_details_after.neighbors_of[0]= strtok(NULL, "$");
	for(i=1; i<udp_details_after.neighbors; i++)
	{
		buffer_con3[i] = strtok(buffer_con2[i], "-");
		udp_details_after.neighbors_of[i]= strtok(NULL, "$");
	}
	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");
	time_stamp(router);
	for(i=0; i<udp_details_after.neighbors; i++)
	{
		fprintf(router, "Neighbor %s has Port Numbers: %s\n",udp_details_after.neighbors_of[i],
				udp_details_after.port_nos_of_nighbors[i]);
	}
	fprintf(router, "-------------------------------------------------------------------------------------------------------------\n");
	if(write(sockfd, "READY", strlen("READY"))<0)      //send ready
		printf("Error in to manager\n");
	time_stamp(router);
	fprintf(router,"READY of %s sent\n", node_assigned);

	char *recv_data_1;
	recv_data_1 = (char *)malloc(sizeof(char *)*10);
	if(read(sockfd,recv_data_1,10)< 0)	 //receive ready ok
	{
		printf("Error in reading\n");
	}
	if(strcmp(recv_data_1, "READY OK") == 0)
	{
		time_stamp(router);
		fprintf(router, "received data at TCP Client is: %s\n", recv_data_1);
	}
	sleep(5);
	for(i=0; i<udp_details_after.neighbors; i++)
	{
		udp_client(udp_details_after.port_nos_of_nighbors[i], "Link_request:");
		sleep(10);
	}
	if(write(sockfd, "Link_establishment_done", strlen("Link_establishment_done"))<0)
		printf("Error in to manager\n");
	time_stamp(router);
	fprintf(router,"Link_establishment_done sent to manager\n");
	bzero(recv_data, strlen(recv_data));
	read1 = read(sockfd,recv_data,500);
	if(read1 < 0)
	{
		printf("Error in reading\n");
	}
	time_stamp(router);
	fprintf(router, "received data at TCP Client is: %s\n", recv_data);
	sleep(10);
	for(i=0; i<udp_details_after.neighbors; i++)
	{
		udp_client(udp_details_after.port_nos_of_nighbors[i], create_LSP);
		sleep(10);
	}
	sleep(20);
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	time_stamp(router);
	fprintf(router, "LSP details received are: \n");
	fprintf(router, "%s\n", LSP_details.LSP_details[udp_details_after.self_node_num]);
	for(i=0; i<udp_details_after.neighbors; i++)
	{

		fprintf(router, "%s\n", LSP_details.LSP_details[atoi(udp_details_after.neighbors_of[i])]);
	}
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");

	char *create_Broadcast;
	create_Broadcast = (char *)malloc(sizeof(char *)* 900);
	strcpy(create_Broadcast, "My_Broadcast:");
	sprintf(node_num, "%d", udp_details_after.self_node_num);
	strcat(create_Broadcast, node_num);
	strcat(create_Broadcast, "b");
	strcat(create_Broadcast, udp_details_after.self_con_table);
	strcat(create_Broadcast, "END");									//Broadcast format: My_Broadcast:<node_num>c<connectivity table>

	strcpy(LSP_details.Broadcast_details[udp_details_after.self_node_num], udp_details_after.self_con_table);
	char *i_char;
	i_char = (char *)malloc(sizeof(char *)* 5);

	for(i=0; i< man_file.number_of_nodes; i++)
	{
		sprintf(i_char, "%d", UDP_port+i);
		if(strcmp(i_char, udp_details_after.self_port) !=0)
		{
			udp_client(i_char, create_Broadcast);
			sleep(10);
		}
	}
	sleep(20);
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	time_stamp(router);
	fprintf(router, "After Broadcast LSPs are:\n");
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		if(i !=udp_details_after.self_node_num)
		{
			fprintf(router, "%s\n", LSP_details.Broadcast_details[i]);
		}
	}
	fprintf(router, "Self LSP: %s\n", LSP_details.Broadcast_details[udp_details_after.self_node_num]);
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");
	fprintf(router, "--------------------------------------------------------------------------------------------------------------\n");

	after_broadcast.cost_array= (int **)malloc(sizeof(int *)*man_file.number_of_nodes);
	for(i=0; i < man_file.number_of_nodes; i++)
	{
		after_broadcast.cost_array[i] = (int *)malloc(sizeof(int)*man_file.number_of_nodes);
	}
	after_broadcast.no_of_lines = (int *)malloc(sizeof(int *)*man_file.number_of_nodes);
	after_broadcast.assigned_port = (char **)malloc(sizeof(char *)*man_file.number_of_nodes);
	for(i=0; i< man_file.number_of_nodes; i++)
	{
		after_broadcast.assigned_port[i] = (char *)malloc(sizeof(char )* 2);
	}
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		after_broadcast.no_of_lines[i] = count_characters(LSP_details.Broadcast_details[i], '\n');
	}
	char **a, **b, **c, **d;
	a = (char **)malloc(sizeof(char *)*man_file.number_of_nodes);
	for(i=0; i < man_file.number_of_nodes; i++)
	{
		a[i] = (char *) malloc(sizeof(char *) *50);
	}
	b= (char **)malloc(sizeof(char *)*man_file.number_of_nodes);
	for(i=0; i < man_file.number_of_nodes; i++)
	{
		b[i] = (char *) malloc(sizeof(char *) *10);
	}
	c= (char **)malloc(sizeof(char *)*man_file.number_of_nodes);
	for(i=0; i < man_file.number_of_nodes; i++)
	{
		c[i] = (char *) malloc(sizeof(char *) *10);
	}
	d= (char **)malloc(sizeof(char *)*man_file.number_of_nodes);
	for(i=0; i < man_file.number_of_nodes; i++)
	{
		d[i] = (char *)malloc(sizeof(char *) *10);
	}
	int j, l, first, second, cost;
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		for(j=0; j<man_file.number_of_nodes; j++)
		{
			after_broadcast.cost_array[i][j] = 999;
		}
	}
	for(j=0; j<man_file.number_of_nodes; j++)
	{
		l=after_broadcast.no_of_lines[j];
		a[0] = strtok(LSP_details.Broadcast_details[j], "\n");
		for(i=1; i<l; i++)
		{
			a[i]=strtok(NULL, "\n");
		}
		for(i=0; i<l; i++)
		{
			b[i]=strtok(a[i], "-");  //first entry
			c[i]=strtok(NULL, "$"); //second entry
			d[i]=strtok(NULL, "p");  //cost
			first = atoi(b[i]);
			second = atoi(c[i]);
			cost = atoi(d[i]);
			after_broadcast.cost_array[first][second] = cost;
			bzero(a[i], strlen(a[i]));
			bzero(b[i], strlen(b[i]));
			bzero(c[i], strlen(c[i]));
			bzero(d[i], strlen(d[i]));
		}
	}
	printf("Dijkstra's algorithm being applied for router no: %d\n", udp_details_after.self_node_num);
	dij();
	time_stamp(router);
	fprintf(router, "Final Routing table is:\n");
	fprintf(router, "To\tCost\tNeighbor\n");
	int to_node;
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		fprintf(router, "%d\t", dijk.confirmed[i][0]);
		fprintf(router, "%d\t", dijk.confirmed[i][1]);
		fprintf(router, "%d\t", dijk.confirmed[i][2]);
		fprintf(router, "\n");
	}
	if(write(sockfd, "Forwarding table created", strlen("Forwarding table created"))<0)
		printf("Error in to manager\n");
	printf("Forwarding table done for router %d\n", udp_details_after.self_node_num);
	time_stamp(router);
	fprintf(router,"Forwarding table created sent to manager\n");
	bzero(recv_data, strlen(recv_data));
	read1 = read(sockfd, recv_data, 500);
	if(read1 < 0)
	{
		printf("Error in reading\n");
	}
	time_stamp(router);
	fprintf(router, "received data at TCP Client is: %s\n", recv_data);
	if(write(sockfd, "start sending data packet details", strlen("start sending data packet details"))<0)
	{
		printf("Error in to manager\n");
	}
	time_stamp(router);
	fprintf(router,"start sending data packet details sent to manager\n");
	printf("Router %d is now ready to receive data packets\n", udp_details_after.self_node_num);
	char *temp_recv, *packet;
	char *temp_null;
	temp_recv = (char *)malloc(sizeof(char*)*100);
	temp_null = (char *)malloc(sizeof(char*)*100);
	packet = (char *)malloc(sizeof(char*)*100);
	bzero(temp_null, strlen(temp_null));
	if(udp_details_after.self_node_num == 0)
	{
		printf("\n\n");
		printf("Forwarding routes for data packets\n");
		printf("\n\n");
	}
	while(1)
	{
		bzero(recv_data, strlen(recv_data));					//   data packet details
		read1 = read(sockfd,recv_data,500);
		if(read1 < 0)
		{
			printf("Error in reading\n");
		}
		time_stamp(router);
		printf("Data received from TCP server is: %s\n", recv_data);
		fprintf(router, "Data received from TCP server is: %s\n", recv_data);
		strcpy(temp_recv, recv_data);
		strtok(temp_recv, ":");
		temp_null = strtok(NULL, "-");
		packet = strtok(NULL, ":");
		if(strcmp(temp_recv, "send_to") == 0)
		{
			to_node = check_forwarding_table(atoi(temp_null));
			if(to_node == udp_details_after.self_node_num)
			{

				//printf("Next node is self neighbor\n");
				for(i=0; i<udp_details_after.neighbors; i++)
				{
					if(atoi(temp_null) == atoi(udp_details_after.neighbors_of[i]))
					{
						fprintf(router, "Next Node: %d\n", atoi(udp_details_after.neighbors_of[i]));
						printf("%s sent to neighbor %d\n", packet, atoi(udp_details_after.neighbors_of[i]));
						fprintf(router, "%s sent to neighbor %d\n", packet, atoi(udp_details_after.neighbors_of[i]));
						udp_client_data(udp_details_after.port_nos_of_nighbors[i], packet);
					}
				}
			}
			for(i=0; i<udp_details_after.neighbors; i++)
			{
				if(to_node == atoi(udp_details_after.neighbors_of[i]))
				{
					fprintf(router, "Next node: %d\n", atoi(udp_details_after.neighbors_of[i]));
					printf("%s sent to neighbor %d\n", recv_data, atoi(udp_details_after.neighbors_of[i]));
					fprintf(router, "%s sent to neighbor %d\n", recv_data, atoi(udp_details_after.neighbors_of[i]));
					udp_client_data(udp_details_after.port_nos_of_nighbors[i], recv_data);
				}
			}
			bzero(temp_recv, strlen(temp_recv));
			bzero(temp_null, strlen(temp_null));
			bzero(packet, strlen(packet));
		}
		else if(strcmp(temp_recv, "exit") == 0)
		{
			//printf("Exit received at TCP client of router number %d\n", udp_details_after.self_node_num);
			break;
		}

	}
	sleep(10);
	printf("Router %d is Exiting\n", p_no);
	close(sockfd);
	fclose(router);
	exit(0);
}

void *thread_for_UDP(void *p)
{
	int port_n, to_node, i;
	port_n = atoi(udp_details_after.self_port);
	int sock;
	struct sockaddr_in server_addr, client_addr;
	struct hostent *host;
	char *recv_data;
	recv_data = (char *)malloc(sizeof(char *)* 900);
	host= (struct hostent *) gethostbyname((char *) "localhost");
	socklen_t len;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("socket error\n");
		exit(1);
	}
	time_stamp(router);
	fprintf(router, "UDP Socket Created\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_n);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),20);
	if(bind(sock,(struct sockaddr *)&server_addr,sizeof(server_addr))<0)
	{
		printf("Bind Failed\n");
		exit(0);
	}
	time_stamp(router);
	fprintf(router, "UDP Socket Binded\n");
	while(1)
	{
		bzero(recv_data, strlen(recv_data));
		len = sizeof(client_addr);
		if(recvfrom(sock, recv_data, 100, 0, (struct sockaddr *)&client_addr, &len)<0)
		{
			printf("error in receiving\n");
			exit(0);
		}

		char *token;
		char *bun;
		char *bus, *bus1, *bus2;
		bus = (char *)malloc(sizeof(char *)*500);
		bus1 = (char *)malloc(sizeof(char *)*500);
		bus2 = (char *)malloc(sizeof(char *)*500);
		token = (char *)malloc(sizeof(char *)*50);
		bun= (char *)malloc(sizeof(char *)*500);
		bzero(bus, strlen(bus));
		bzero(bus1, strlen(bus1));
		bzero(bus2, strlen(bus2));
		bzero(bun, strlen(bun));
		strcpy(bun, recv_data);
		token = strtok(bun, ":");

		if(strcmp(token, "Link_request") == 0)
		{
			time_stamp(router);
			fprintf(router, "data received at UDP server is %s\n", recv_data);
			sendto(sock, "Link_establishment_ACK", strlen("Link_establishment_ACK"), 0,
					(struct sockaddr *)&client_addr, sizeof(client_addr));
			time_stamp(router);
			fprintf(router, "Link_establishment_ACK sent");
		}
		else if(strcmp(token, "My_LSP") == 0)
		{
			time_stamp(router);
			fprintf(router, "data received at UDP server is %s\n", recv_data);
			strcpy(bus, recv_data);
			strtok(bus, ":");
			bus1 = strtok(NULL, "c");
			bus2 = strtok(NULL, "END");
			int asd;
			asd = atoi(bus1);
			strcpy(LSP_details.LSP_details[asd], bus2);
			sendto(sock,"LSP receive Success",strlen("LSP receive Success"),0,
					(struct sockaddr *)&client_addr,sizeof(client_addr));
			time_stamp(router);
			fprintf(router, "LSP receive Success sent");
		}
		else if(strcmp(token, "My_Broadcast") == 0)
		{
			strcpy(bus, recv_data);
			strtok(bus, ":");
			bus1 = strtok(NULL, "b");
			bus2 = strtok(NULL, "END");
			int asd;
			asd = atoi(bus1);
			strcpy(LSP_details.Broadcast_details[asd], bus2);
			sendto(sock,"Broadcast receive Success",strlen("Broadcast receive Success"),0,
					(struct sockaddr *)&client_addr,sizeof(client_addr));

		}
		else if(strcmp(token, "send_to") == 0)
		{
			time_stamp(router);
			fprintf(router, "data received at UDP server is %s\n", recv_data);
			strcpy(bus, recv_data);
			strtok(bus, ":");
			bus1 = strtok(NULL, "-");
			bus2 = strtok(NULL, ":");
			to_node = check_forwarding_table(atoi(bus1));
			if(to_node == udp_details_after.self_node_num)
			{
				//printf("Next node is self neighbor\n");
				for(i=0; i<udp_details_after.neighbors; i++)
				{
					if(atoi(bus1) == atoi(udp_details_after.neighbors_of[i]))
					{
						fprintf(router, "Next Node: %d\n", atoi(udp_details_after.neighbors_of[i]));
						printf("%s sent to neighbor %d\n", bus2, atoi(udp_details_after.neighbors_of[i]));
						fprintf(router, "%s sent to neighbor %d\n", bus2, atoi(udp_details_after.neighbors_of[i]));
						udp_client_data(udp_details_after.port_nos_of_nighbors[i], bus2);
					}
				}

			}
			else
			{
				for(i=0; i<udp_details_after.neighbors; i++)
				{
					if(to_node == atoi(udp_details_after.neighbors_of[i]))
					{
						fprintf(router, "Next node: %d\n", atoi(udp_details_after.neighbors_of[i]));
						printf("%s sent to neighbor %d\n", recv_data, atoi(udp_details_after.neighbors_of[i]));
						fprintf(router, "%s sent to neighbor %d\n", recv_data, atoi(udp_details_after.neighbors_of[i]));
						udp_client_data(udp_details_after.port_nos_of_nighbors[i], recv_data);
					}
				}
			}

		}
		else
		{
			printf("\n\n");
			printf("FINAL DATA PACKET RECEIVED AT ROUTER NUMBER %d AND THE MESSAGE IS : %s\n", udp_details_after.self_node_num, recv_data);
			printf("*****************************************************************************\n\n");
			fprintf(router, "*****************************************************************************\n\n");
			fprintf(router, "DATA PACKET RECIEVED\n THE MESSAGE IS: %s\n", recv_data);
			fprintf(router, "*****************************************************************************\n");
		}
	}
	return 0;
}

void udp_client_data(char *port_no, char *message)
{
	int server_port;
	server_port = atoi(port_no);
	int sockfd,n;
	struct sockaddr_in serv_addr;
	struct hostent *host;
	char* recv;
	char *ip;
	ip = (char *)malloc(sizeof(char *)*20);
	recv = (char *)malloc(sizeof(char *)*200);
	strcpy(ip, "localhost");
	host = (struct hostent *) gethostbyname((char *)ip);
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
	serv_addr.sin_port=htons(server_port);
	socklen_t serverlen;
	serverlen = sizeof(serv_addr);
	sendto(sockfd,message,strlen(message),0,
			(struct sockaddr *)&serv_addr,sizeof(serv_addr));
}

void udp_client(char *port_no, char *message)
{
	int server_port;
	server_port = atoi(port_no);
	int sockfd,n;
	struct sockaddr_in serv_addr;
	struct hostent *host;
	char* recv;
	char *ip;
	ip = (char *)malloc(sizeof(char *)*20);
	recv = (char *)malloc(sizeof(char *)*200);
	strcpy(ip, "localhost");
	host = (struct hostent *) gethostbyname((char *)ip);
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
	serv_addr.sin_port=htons(server_port);
	socklen_t serverlen;
	serverlen = sizeof(serv_addr);
	sendto(sockfd,message,strlen(message),0,
			(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	n=recvfrom(sockfd,recv,200,0,(struct sockaddr *)&serv_addr,&serverlen);
	if(n<0)
	{
		printf("Error reading from socket\n");
		exit(0);
	}
}

void dij()
{
	int **buffer;
	int i,j,k,routers = man_file.number_of_nodes, confirmed_flag=0, buffer_flag=0,buffer_equal_index, break_flag=0;
	int self_id, next, prev, index, index_tent, m, least_cost, tent_index, n, s;

	self_id = udp_details_after.self_node_num;

	dijk.confirmed = (int **)malloc(sizeof(int*)*routers);
	for(i=0;i<routers;i++)
	{
		dijk.confirmed[i] = (int *)malloc(sizeof(int)*3);
	}
	dijk.tentative = (int **)malloc(sizeof(int*)*routers);
	for(i=0;i<routers;i++)
	{
		dijk.tentative[i] = (int *)malloc(sizeof(int)*3);
	}
	buffer = (int **)malloc(sizeof(int*)*routers);
	for(i=0;i<routers;i++)
	{
		buffer[i] = (int *)malloc(sizeof(int)*3);
	}
	dijk.cost = (int **)malloc(sizeof(int*)*routers);
	for(i=0;i<routers;i++)
	{
		dijk.cost[i] = (int *)malloc(sizeof(int)*routers);
	}

	for(i=0; i<man_file.number_of_nodes; i++)
	{
		for(j=0; j<man_file.number_of_nodes; j++)
		{
			dijk.cost[i][j] = after_broadcast.cost_array[i][j];
		}
	}
	for(i=0;i<routers;i++)
	{
		for(j=0;j<3;j++)
		{
			dijk.confirmed[i][j] = 1111;
			dijk.tentative[i][j] = 1111;
			buffer[i][j] = 1111;
		}
	}
	dijk.confirmed[0][0] = self_id;                   //neighbor
	dijk.confirmed[0][1] = 0;                         //cost
	dijk.confirmed[0][2] = self_id;                   //next_hop

	next = dijk.confirmed[0][0];
	prev = dijk.confirmed[0][0];
	index = 1;
	index_tent = 1;
	for(n=0; n<routers*routers; n++)
	{
		k=0;
		for(j=0;j<routers;j++)
		{
			if((dijk.cost[next][j] < 1111) && (j != next))
			{
				buffer[k][0] = j;
				buffer[k][1] = dijk.cost[next][j];
				if(prev == self_id)
				{
					buffer[k][2] = next;
				}
				else
				{
					buffer[k][2] = prev;
				}
				k++;
			}
		}
		for(j=0;j<k;j++)
		{
			buffer[j][1] = buffer[j][1] + dijk.confirmed[index-1][1];           //cost added
			for(m=0; m<index; m++)
			{
				if(buffer[j][0] == dijk.confirmed[m][0])
				{
					confirmed_flag =1;
					break;
				}
				else
				{
					confirmed_flag = 0;
				}
			}
			if(confirmed_flag == 0)
			{
				break_flag = 1;
				for(s=0; s<index_tent; s++)
				{
					if(buffer[j][0] == dijk.tentative[s][0])
					{
						buffer_flag=1;
						buffer_equal_index = s;
					}
				}
				if(buffer_flag == 1)
				{
					if(buffer[j][1] < dijk.tentative[buffer_equal_index][1])
					{
						dijk.tentative[buffer_equal_index][0] = buffer[j][0];
						dijk.tentative[buffer_equal_index][1] = buffer[j][1];
						dijk.tentative[buffer_equal_index][2] = buffer[j][2];
					}
				}
				else
				{
					dijk.tentative[index_tent-1][0] = buffer[j][0];
					dijk.tentative[index_tent-1][1] = buffer[j][1];
					dijk.tentative[index_tent-1][2] = buffer[j][2];
					index_tent++;
				}
				buffer_flag=0;
			}
			confirmed_flag = 0;
		}
		if(index_tent == 1 && break_flag == 1)
		{
			break;
		}
		if(dijk.tentative[0][1]< dijk.tentative[1][1])
		{
			least_cost = dijk.tentative[0][1]; 					//least cost between first two
			tent_index = 0;
		}
		else
		{
			least_cost= dijk.tentative[1][1];
			tent_index = 1;
		}
		for (i=2; i<index_tent-1; i++)
		{

			if(dijk.tentative[i][1] < least_cost)
			{
				least_cost = dijk.tentative[i][1];
				tent_index = i;
			}
		}
		dijk.confirmed[index][0] = dijk.tentative[tent_index][0];
		dijk.confirmed[index][1] = dijk.tentative[tent_index][1];
		dijk.confirmed[index][2] = dijk.tentative[tent_index][2];
		prev = dijk.confirmed[index][2];
		next = dijk.confirmed[index][0];
		index++;

		for(i=tent_index; i<index_tent-1; i++)
		{
			dijk.tentative[i][0] = dijk.tentative[i+1][0];
			dijk.tentative[i][1] = dijk.tentative[i+1][1];
			dijk.tentative[i][2] = dijk.tentative[i+1][2];
		}
		index_tent--;
		if(index_tent == 1 && confirmed_flag == 1)
		{
			break;
		}
	}
}

int check_forwarding_table(int node)
{
	int i, to_node;
	for(i=0; i<man_file.number_of_nodes; i++)
	{
		if(node == dijk.confirmed[i][0])
		{
			to_node = dijk.confirmed[i][2];
		}
	}
	return to_node;
}
