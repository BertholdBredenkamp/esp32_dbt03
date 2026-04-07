#include "applications.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"


#include "settings.h"
#include "wlan.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT_LED    GPIO_NUM_8

int app_btx(io_type_t *io)
{
	//40x25 with wrap-arround
	io->out(0x1f);
	io->out(0x2d);
	//Reset to serial attributes mode
	io->out(0x1f);
	io->out(0x2f);
	io->out(0x41);
  // Bre eingeschaltet wieder aus
	// app_write_string(io, "\x1B\x22\x41"); //Invoke C1P set
	char addr_str[128];
	int addr_family;
	int ip_protocol;
	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = inet_addr(get_setting("SRVIP"));
	printf("Bre Settig IP : %s\n", get_setting("SRVIP"));
	int port=atoi(get_setting("SVRPORT"));
	// dest_addr.sin_addr.s_addr = inet_addr("195.201.94.166");
  // dest_addr.sin_addr.s_addr = inet_addr("192.168.10.161");
  // Port 20000
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
  printf("Server-IP %s ",get_setting("SRVIP"));
  printf("Server-Port %i \n",port);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

// DNS-Auflösung
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM;


	if (getaddrinfo(get_setting("SRVIP"), get_setting("SVRPORT"), &hints, &res) != 0) {
    printf("Fehler bei DNS-Auflösung\n");
  }
	// IP-Adresse extrahieren und ausgeben
  struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	printf("IP nach DNS: %s\n", inet_ntoa(*addr));
	// addr_str = inet_ntoa(*addr);
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
  printf("IP nach DNS 2: %s\n", str);
	//
	dest_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*addr));

	inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

	int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
	if (sock < 0) {
		app_init_screen(io);
		app_gotoxy(io, 1,1);
		app_write_string(io, "Socket Creation Failed\x1a");
    printf("Socket Fehler!");
		return -1;
	}

	int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err != 0) {
		app_init_screen(io);
		app_gotoxy(io, 1,1);
		app_write_string(io, "Connect Failed Failed\x1a");
    printf("Connect Fehler!");
		return -2;
	}
	/* set recv timeout (100 ms) */
	int opt = 100;
	lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));

	while (io->status(-1)==0) {
		char tx_buffer[8]; //Terminal=>IP
		int tx_len = io->in(0);
		if (tx_len>=0) {
			tx_buffer[0]=tx_len;
			int err = send(sock, tx_buffer, 1, 0);
			if (err < 0) {
				break;
			}
		}

		char rx_buffer[8]; //IP=>Terminal
		int rx_len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
		// Error occurred during receiving
		if (rx_len < 0) {
			if (errno!=11) {
				break;
			}
		}
		// Data received
		else {
			int n;
			for (n=0; n<rx_len; n++) io->out(rx_buffer[n]);
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
	shutdown(sock, 0);
	close(sock);
	return 0;
}




int application(io_type_t *io)
{	
	app_init_screen(io);
	app_gotoxy(io, 1,1);
	app_write_string(io, "5 for settings\x1a");
	int cnt=0;
	int i=0;
	while ((i=io->in(0))<0) {
		vTaskDelay(100/portTICK_PERIOD_MS);
		cnt=cnt+1;
    printf("Count : %i\n",cnt);
		if (cnt>10) break;
	}
	if (i=='5') {
		settings_app(io);
	} 
	//app_gotoxy(io, 1,1);
	//vTaskDelay(20000/portTICK_PERIOD_MS);
	return app_btx(io);
}


void app_write_string(const io_type_t *io, const char *s)
{
	if (s[0]==0) return;
	io->out(s[0]);
	return app_write_string(io, &(s[1]));
}

void app_status_string(const io_type_t *io, const char *s)
{
	app_write_string(io, "\x1f\x2f\x40\x58"); //Service jump to line 24
	app_write_string(io, s);
	app_write_string(io, "\x1f\x2f\x4f"); //Service jump back

}


void app_init_screen(const io_type_t *io)
{		
	//40x25 with wrap-arround
	io->out(0x1f);
	io->out(0x2d);
	//Reset to parallel attributes mode
	io->out(0x1f);
	io->out(0x2f);
	io->out(0x42);
	app_write_string(io, "\x1B\x22\x41"); //Invoke C1P set
}

void app_set_palette(const io_type_t *io, const int palette)
{
	io->out(0x9b);
	io->out(0x30+palette);
	io->out(0x40);
}

void app_set_screen_colour(const io_type_t *io, const int colour)
{
	app_set_palette(io, colour/8);
	app_write_string(io, "\x1b\x23\x20");
	io->out(0x50+colour%8);
}

void app_set_line_colour(const io_type_t *io, const int colour)
{
	app_set_palette(io, colour/8);
	app_write_string(io, "\x1b\x23\x21");
	io->out(0x50+colour%8);
}


void app_set_bg_colour(const io_type_t *io, const int colour)
{
	app_set_palette(io, colour/8);
	io->out(0x90+colour%8);
}


void app_gotoxy(const io_type_t *io, const int x, const int y)
{
	io->out(0x1f);
	io->out(0x41+y);
	io->out(0x41+x);
}

// const TickType_t xDelay = 100 / portTICK_PERIOD_MS;

void terminal_task(void *pvParameters)
{
	if (pvParameters==NULL) vTaskDelete(NULL);
	printf("task started\n");
	io_type_t *io=(io_type_t*) pvParameters;
	while (0==0) {
		printf("init terminal\n");
		io->status(1); //init Terminal
		printf("starting settings app\n");
	
		app_init_screen(io);
		app_gotoxy(io, 1,1);
		app_write_string(io, "Taste 5 fuer Einstellungen\x1a");
		int cnt=0;
		int i=0;
		//
		// Bre - Zuerst wird immer eine "1" in i gelesen, ev. vom Terminal abhängig
		// 
		while ((i=io->in(0)) <0) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
			// printf("Settings IO: %i\n", i);
			app_gotoxy(io, 1,2);
			app_write_string(io, "Status: ");
			app_write_string(io, wlan_status_string);
			app_write_string(io, " \x18");
      printf("WLAN Status 2 %s\n", wlan_status_string);
			// vTaskDelay(xDelay);
			if (wlan_status==1) {
 				char s[4];
				s[0]='0'+cnt/10;
				s[1]='0'+cnt%10;
				s[2]=0x1a;
				s[3]=0;
				app_write_string(io, s);
				cnt=cnt+1;
				if (cnt>10) break;
			}
		}
		printf("Eingabe %i\n", i);
		if (i=='5') {
			settings_app(io);
			if (wlan_status == 0) {
			  esp_restart();
		  }
			// wifi_init_sta();		// Versuch ob es hier erneut geht
      printf("Settings App1\n");
		} 
		// Blause LED einschalten wenn WLAN verbunden
		if (wlan_status == 1) {
			gpio_set_level(GPIO_OUTPUT_LED, 0);
		  // digitalWrite(GPIO_OUTPUT_LED, 1);
		}
		else {
			gpio_set_level(GPIO_OUTPUT_LED, 1);
			// digitalWrite(GPIO_OUTPUT_LED, 0);
		}
		
		app_btx(io);
		io->status(2); //DeInit Terminal
	}
	vTaskDelete(NULL);
  printf("task beendet\n");
}
