
void kmain()
{
	// hier ist der Kernel!
	
	// testausgabe
	short* bildschirm = (short*)0xB8000;
	*bildschirm = 0x0f << 8 | 'a';


}
