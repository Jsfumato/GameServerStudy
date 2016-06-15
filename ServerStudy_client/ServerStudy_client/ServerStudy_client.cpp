#include "Client.h"

int main()
{
	Client c1 = Client();

	do 
	{
		c1.SetServerIP();
		c1.SetServerPort();
	} 
	while (c1.ConnectServer() == false);
	
	c1.SendMassageToServer();
	
	return 0;
}