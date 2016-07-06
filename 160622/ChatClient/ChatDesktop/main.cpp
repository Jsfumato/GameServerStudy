#include <iostream>
#include <thread>
#include "MainForm.h"

int main()
{
	MainForm mainForm;

	mainForm.Init();

	mainForm.CreateGUI();
	
	mainForm.ShowModal();

	return 0;
}