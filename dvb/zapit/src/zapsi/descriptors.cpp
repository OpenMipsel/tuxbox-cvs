#include <stdio.h>
#include <map>
#include <string>
#include <map.h>
#include "sdt.h"
#include "scan.h"

std::map<int,transpondermap> scantransponders;
std::map<int,scanchannel> scanchannels;
multimap<std::string, bouquet_mulmap> scanbouquets;
std::string curr_chan_name;
int found_transponders;
int found_channels;
char last_provider[100];

uint8_t stuffing_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t linkage_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t priv_data_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t network_name_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	std::string name;

	for (pos = 0; pos < descriptor_length; pos++)
		name += buffer[pos + 2];

	printf("Network-name: %s\n", name.c_str());
#endif
	return descriptor_length;
}

uint8_t service_list_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	uint16_t service_id;
	uint8_t service_type;

	for (pos = 2; pos < descriptor_length + 2; pos += 3)
	{
		service_id = (buffer[pos] << 8) | buffer[pos + 1];
		service_type = buffer[pos + 2];
		printf("service_id: %04x\n", service_id);
		printf("service_type: %04d\n", service_type);
	}
#endif
	return descriptor_length;
}

uint8_t cable_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id)
{
	uint8_t descriptor_length = buffer[1];
	uint32_t frequency = ((buffer[2] >> 4) * 10000) + ((buffer[2] & 0x0F) * 1000) + ((buffer[3] >> 4) * 100) + ((buffer[3] & 0x0F) * 10) + (buffer[4] >> 4);
	uint8_t FEC_outer = buffer[7] & 0x0F;
	uint8_t modulation = buffer[8];
	uint32_t symbol_rate = ((buffer[9] >> 4) * 100000) + ((buffer[9] & 0x0F) * 10000) + ((buffer[10] >> 4) * 1000) + ((buffer[10] & 0x0F) * 100) + ((buffer[11] >> 4) * 10) + (buffer[11] & 0x0F);
	uint8_t FEC_inner = buffer[12] & 0x0F;

	printf("[descriptor.cpp] frequency: %d\n", frequency);
	printf("[descriptor.cpp] modulation %d, symbol_rate %d, FEC_inner %d, FEC_outer %d\n", modulation, symbol_rate, FEC_inner, FEC_outer);

	if (FEC_inner == 15)
		FEC_inner = 0;

	if (scantransponders.count(transport_stream_id) == 0)
	{
		printf("[descriptor.cpp] new transponder - transport_stream_id: %04x\n", transport_stream_id);
		found_transponders++;
		eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_transponders, sizeof(found_transponders));
		scantransponders.insert(std::pair<int, transpondermap>(transport_stream_id, transpondermap(transport_stream_id, frequency, symbol_rate, FEC_inner)));
	}

	return descriptor_length;
}

uint8_t sat_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id, int diseqc)
{
	uint8_t descriptor_length = buffer[1];
	uint32_t frequency = ((buffer[2] >> 4) * 100000) + ((buffer[2] & 0x0F) * 10000) + ((buffer[3] >> 4) * 1000) + ((buffer[3] & 0x0F) * 100) + ((buffer[4] >> 4) * 10) + (buffer[4]&0x0F);
	uint16_t orbital_position = ((buffer[6] >> 4) * 1000) + ((buffer[6] & 0x0F) * 100) + ((buffer[7] >> 4) * 10) + (buffer[7] & 0x0F);
	uint8_t west_east_flag = (buffer[8] >> 7) & 0x01;
	uint8_t polarization = (buffer[8] >> 5) & 0x03;
	uint8_t modulation = buffer[8] & 0x1F;
	uint32_t symbol_rate = ((buffer[9] >> 4) * 100000) + ((buffer[9] & 0x0F) * 10000) + ((buffer[10] >> 4) * 1000) + ((buffer[10] & 0x0F) * 100) + ((buffer[11] >> 4) * 10) + (buffer[11] & 0x0F);
	uint8_t FEC_inner = buffer[12] & 0x0F;

	printf("[descriptor.cpp] orbital_position: %.1f %s\n", orbital_position * 0.1, west_east_flag ? "East" : "West");
	printf("[descriptor.cpp] frequency: %d %s\n", frequency, polarization ? "V" : "H");
	printf("[descriptor.cpp] modulation %d, symbol_rate %d, FEC_inner %d\n", modulation, symbol_rate, FEC_inner);

	if (scantransponders.count(transport_stream_id) == 0)
	{
		printf("[descriptor.cpp] new transponder - transport_stream_id: %04x\n", transport_stream_id);
		found_transponders++;
		eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_transponders, sizeof(found_transponders) );
		scantransponders.insert(std::pair<int, transpondermap>(transport_stream_id, transpondermap(transport_stream_id, frequency, symbol_rate, FEC_inner, polarization, diseqc)));
	}

	return descriptor_length;
}

uint8_t terr_deliv_system_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t multilingual_network_name_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t freq_list_desc(uint8_t *buffer)
{
	uint8_t descriptor_length  = buffer[1];
#if 0
	int current = 3;
	printf("Coding-type: %d\n", (buffer[2]&0x3));
	while (current < len)
		printf("Center Frequency: %d", (buffer[++current]<<24)|(buffer[++current]<<16)|(buffer[++current]<<8)|buffer[++current]);
#endif
	return descriptor_length;
}

uint8_t cell_list_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t cell_freq_list_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t announcement_support_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t service_name_desc(uint8_t *buffer, uint16_t service_id, uint16_t transport_stream_id, uint16_t original_network_id, bool scan_mode)
{
	uint8_t descriptor_length = buffer[1];
	uint8_t service_type = buffer[2];
	uint8_t service_provider_name_length = buffer[3];
	//uint8_t service_name_length = buffer[service_provider_name_length + 4];

	uint16_t pos;
	char val[5];

	std::string provname;
	std::string servicename;
	std::map<int, scanchannel>::iterator I = scanchannels.find((transport_stream_id << 16) | service_id);

	for (pos = 4; pos < service_provider_name_length + 4; pos++)
	{
		switch (buffer[pos])
		{
		case '&':
			provname += "&amp;";
       			break;
		case '<':
			provname += "&lt;";
			break;
		case '\"':
			provname += "&quot;";
			break;
		case 0x81:
		case 0x82:
			break;
		case 0x86:
			//provname += "<b>";
			break;
		case 0x87:
			//provname += "</b>";
			break;
		case 0x8a:
			//provname += "<br/>";
			break;
		default:
			if (buffer[pos] < 32)
				break;
			if ((buffer[pos] >= 32) && (buffer[pos] < 128))
				provname += buffer[pos];
			else if (buffer[pos] == 128)
				break;
			else
			{
				sprintf(val, "%d", buffer[pos]);
				provname += "&#";
				provname += val;
				provname += ";";
			}
		}
	}

	for (pos++; pos < descriptor_length + 2; pos++)
	{
		switch (buffer[pos])
		{
     		case '&':
			servicename += "&amp;";
			break;
		case '<':
			servicename += "&lt;";
			break;
		case '\"':
			servicename += "&quot;";
			break;
		case 0x81:
		case 0x82:
			break;
		case 0x86:
			//servicename += "<b>";
			break;
		case 0x87:
			//servicename += "</b>";
			break;
		case 0x8a:
			//servicename += "<br/>";
			break;
		default:
			if (buffer[pos] < 32)
				break;
			if ((buffer[pos] >= 32) && (buffer[pos] < 128))
				servicename += buffer[pos];
			else if (buffer[pos] == 128)
				break;
			else
			{
				sprintf(val, "%d", buffer[pos]);
				servicename += "&#";
				servicename += val;
				servicename += ";";
			}
		}
	}

	printf("provider: %s\n", provname.c_str());
	printf("service: %s\n", servicename.c_str());

	if (scan_mode)
	{
		if (scanchannels.count((transport_stream_id << 16) | service_id) != 0)
		{
			printf("Found a channel in map\n");
			I->second.name = servicename;
			I->second.onid = original_network_id;
			I->second.service_type = service_type;
		}
		else
		{
			found_channels++;
			eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_channels, sizeof(found_channels));
			scanchannels.insert(std::pair<int, scanchannel>((transport_stream_id << 16) | service_id, scanchannel(servicename, service_id, transport_stream_id, original_network_id, service_type)));
		}

		if (provname == "")
			provname = "Unknown Provider";

		if (strcmp(last_provider, provname.c_str()) != 0)
		{
			strcpy(last_provider, provname.c_str());
			eventServer->sendEvent(CZapitClient::EVT_SCAN_PROVIDER, CEventServer::INITID_ZAPIT, last_provider, strlen(last_provider) + 1);
		}

		if ((service_type == 1) || (service_type == 2) || (service_type == 4) || (service_type == 5))
			scanbouquets.insert(std::pair<std::string, bouquet_mulmap>(provname.c_str(), bouquet_mulmap(provname, servicename, service_id, original_network_id)));
	}

	return descriptor_length;
}

uint8_t bouquet_name_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	std::string name;

	for (pos = 0; pos < descriptor_length; pos++)
		name += buffer[pos + 2];

	printf("Bouquet name: %s\n", name.c_str());
#endif
	return descriptor_length;
}



uint8_t country_availability_desc(uint8_t *buffer)
{
	return buffer[1];
}


uint8_t nvod_ref_desc(uint8_t *buffer, uint16_t transport_stream_id, bool scan_mode)
{
	return buffer[1];
}

uint8_t time_shift_service_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t mosaic_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t ca_ident_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t telephone_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t multilingual_service_name_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t data_broadcast_desc(uint8_t *buffer)
{
	return buffer[1];
}

