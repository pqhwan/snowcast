
	int
main ( int argc, char *argv[] )
{
	unsigned char buf[1024];
	uint16_t packetSize;
	char *songname = "monkey";
	char *replyString = "wrong";

	//packing hello
	packetSize = pack(buf, "ch",(uint8_t)0, (uint16_t) 4444);
	
	
	//unpacking hello
	uint8_t commandType;
	uint16_t udpPort;
	unpack(buf, "ch", commandType, udpPort);

	//packing setstation
	packetSize = pack(buf, "ch", (uint8_t)1, (uint16_t)2);
	
	//unpacking setstation
	uint8_t commandType;
	uint16_t stationNumber;
	unpack(buf, "ch", &commandType, &stationNumber);
		
	//packing welcome
	packetSize = pack(buf, "ch", (uint8_t) 0, (uint16_t) 20);
	
	//unpacking welcome
	uint8_t replyType;
	uint16_t numStations;
	unpack(buf, "ch", &replyType, &numStations);
	
	//packing announce
	packetSize = pack(buf, "ccs", (uint8_t)1, (uint8_t)5, songname);
	
	//unpacking announce
	uint8_t replyType;
	uint8_t songnameSize;
	char songname[songnameSize];
	unpack(buf, "ccs", &replyType, &songnameSize, songname);
	
	//packing invalidcommand
	packetSize = pack(buf, "ccs", (uint8_t)2, (uint8_t)5, replyString);
	
	//unpacking invalidcommand
	uint8_t replyType;
	uint8_t replyStringSize;
	char replyString[replyStringSize];
	unpack(buf, "ccs", &replyType, &replyStringSize, replyString);
	
	return EXIT_SUCCESS;
}
