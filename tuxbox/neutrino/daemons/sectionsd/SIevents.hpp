#ifndef SIEVENTS_HPP
#define SIEVENTS_HPP
//
// $Id: SIevents.hpp,v 1.5 2001/05/19 20:15:08 fnbrd Exp $
//
// classes SIevent and SIevents (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Log: SIevents.hpp,v $
// Revision 1.5  2001/05/19 20:15:08  fnbrd
// Kleine Aenderungen (und epgXML).
//
// Revision 1.4  2001/05/18 20:31:04  fnbrd
// Aenderungen fuer -Wall
//
// Revision 1.3  2001/05/18 13:11:46  fnbrd
// Fast komplett, fehlt nur noch die Auswertung der time-shifted events
// (Startzeit und Dauer der Cinedoms).
//
// Revision 1.2  2001/05/17 01:53:35  fnbrd
// Jetzt mit lokaler Zeit.
//
// Revision 1.1  2001/05/16 15:23:47  fnbrd
// Alles neu macht der Mai.
//
//

// forward references
class SIservice;

struct eit_event {
  unsigned short event_id : 16;
  unsigned long long start_time : 40;
  unsigned int duration : 24;
  unsigned char running_status : 3;
  unsigned char free_CA_mode : 1;
  unsigned short descriptors_loop_length : 12;
} __attribute__ ((packed)) ;


struct descr_component_header {
  unsigned char descriptor_tag : 8;
  unsigned char descriptor_length : 8;
  unsigned char reserved_future_use : 4;
  unsigned char stream_content : 4;
  unsigned char component_type : 8;
  unsigned char component_tag : 8;
  unsigned iso_639_2_language_code : 24;
} __attribute__ ((packed)) ;

class SIcomponent {
  public:
    SIcomponent(const struct descr_component_header *comp) {
      streamContent=comp->stream_content;
      componentType=comp->component_type;
      componentTag=comp->component_tag;
      if(comp->descriptor_length>sizeof(struct descr_component_header)-2)
        component=string(((const char *)comp)+sizeof(struct descr_component_header), comp->descriptor_length-(sizeof(struct descr_component_header)-2));
    }
    // Der Operator zum sortieren
    bool operator < (const SIcomponent& c) const {
      return streamContent < c.streamContent;
//      return component < c.component;
    }
    void dump(void) const {
      if(component.length())
        printf("Component: %s\n", component.c_str());
      printf("Stream Content: 0x%02hhx\n", streamContent);
      printf("Component type: 0x%02hhx\n", componentType);
      printf("Component tag: 0x%02hhx\n", componentTag);
    }
    int saveXML(FILE *file) const {
      if(fprintf(file, "    <component tag=\"0x%02hhx\" type=\"0x%02hhx\" stream_content=\"0x%02hhx\" />\n", componentTag, componentType, streamContent)<0)
        return 1;
      return 0;
    }
    string component; // Text aus dem Component Descriptor
    unsigned char componentType; // Component Descriptor
    unsigned char componentTag; // Component Descriptor
    unsigned char streamContent; // Component Descriptor
};

// Fuer for_each
struct printSIcomponent : public unary_function<SIcomponent, void>
{
  void operator() (const SIcomponent &c) { c.dump();}
};

// Fuer for_each
struct saveSIcomponentXML : public unary_function<SIcomponent, void>
{
  FILE *f;
  saveSIcomponentXML(FILE *fi) { f=fi;}
  void operator() (const SIcomponent &c) { c.saveXML(f);}
};


typedef multiset <SIcomponent, less<SIcomponent> > SIcomponents;

class SIevent {
  public:
    SIevent(const struct eit_event *);
    // Std-Copy
    SIevent(const SIevent &);
    unsigned short eventID;
    string name; // Name aus dem Short-Event-Descriptor
    string text; // Text aus dem Short-Event-Descriptor
    string itemDescription; // Aus dem Extended Descriptor
    string item; // Aus dem Extended Descriptor
    string extendedText; // Aus dem Extended Descriptor
    string contentClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
    string userClassification; // Aus dem Content Descriptor, als String, da mehrere vorkommen koennen
    time_t startzeit; // lokale Zeit, 0 -> time shifted (cinedoms)
    unsigned dauer; // in Sekunden, 0 -> time shifted (cinedoms)
    unsigned short serviceID;
    SIcomponents components;
    // Der Operator zum sortieren
    bool operator < (const SIevent& e) const {
      // Erst nach Service-ID, dann nach Event-ID sortieren
      return serviceID!=e.serviceID ? serviceID < e.serviceID : eventID < e.eventID;
    }
    int saveXML(FILE *file) const { // saves the event
      return saveXML0(file) || saveXML2(file);
    }
    int saveXML(FILE *file, const char *serviceName) const; // saves the event
    void dump(void) const; // dumps the event to stdout
    void dumpSmall(void) const; // dumps the event to stdout (not all information)
  protected:
    int saveXML0(FILE *f) const;
    int saveXML2(FILE *f) const;
};

// Fuer for_each
struct printSIevent : public unary_function<SIevent, void>
{
  void operator() (const SIevent &e) { e.dump();}
};

// Fuer for_each
struct saveSIeventXML : public unary_function<SIevent, void>
{
  FILE *f;
  saveSIeventXML(FILE *fi) { f=fi;}
  void operator() (const SIevent &e) { e.saveXML(f);}
};

// Fuer for_each
struct saveSIeventXMLwithServiceName : public unary_function<SIevent, void>
{
  FILE *f;
  const SIservices *s;
  saveSIeventXMLwithServiceName(FILE *fi, const SIservices &svs) {f=fi; s=&svs;}
  void operator() (const SIevent &e) {
    SIservices::iterator k=s->find(SIservice(e.serviceID));
    if(k!=s->end()) {
      if(k->serviceName.length())
      e.saveXML(f, k->serviceName.c_str());
    }
    else
      e.saveXML(f);
  }
};

// Fuer for_each
struct printSIeventWithService : public unary_function<SIevent, void>
{
  printSIeventWithService(const SIservices &svs) { s=&svs;}
  void operator() (const SIevent &e) {
    SIservices::iterator k=s->find(SIservice(e.serviceID));
    if(k!=s->end())
      printf("%s\n", k->serviceName.c_str());
    e.dumpSmall();
  }
  const SIservices *s;
};

typedef set <SIevent, less<SIevent> > SIevents;

#endif // SIEVENTS_HPP
