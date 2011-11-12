/*fare il merge di questa funzione nel file dbAccess.c, dove c'e' gia' il collegamento con il DB e tutte le variabili sono ok */

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "libpq-fe.h"
//#include "../include/ping.h"
//#include "../include/dbAccess.h"
#include "../include/config.h"
#include "../include/hashtbl.h"

#define APPLICATION_SERVER_NAME		"Application Server"
// "Node" ELEMENTS 
#define REG_ELEMENT_REGISTRATION	"Registration"
#define REG_ELEMENT_NODE		"Node"
#define REG_ELEMENT_GEOLOCATION		"GeoLocation"
#define REG_ELEMENT_NODEMANAGER		"NodeManager"
#define REG_ELEMENT_GRIDFTPSERVICE	"GridFTPService"
#define REG_ELEMENT_CONFIGURATION	"Configuration"

// "Node" ATTRIBUTES
#define REG_ATTR_NODE_ORGANIZATION		 	"organization"
#define REG_ATTR_NODE_NAMESPACE 			"namespace"
#define REG_ATTR_NODE_NODEPEERGROUP			"nodePeerGroup"
#define REG_ATTR_NODE_SUPPORTEMAIL 			"supportEmail"
#define REG_ATTR_NODE_NODEHOSTNAME			"hostname"
#define REG_ATTR_NODE_NODEIP				"ip"
#define REG_ATTR_NODE_DN				"dn"
#define REG_ATTR_NODE_SHORTNAME				"shortName"
#define REG_ATTR_NODE_LONGNAME				"longName"
#define REG_ATTR_NODE_TIMESTAMP				"timeStamp"
#define REG_ATTR_NODE_VERSION				"version"
#define REG_ATTR_NODE_RELEASE				"release"
#define REG_ATTR_NODE_NODETYPE				"nodeType"
#define REG_ATTR_NODE_ADMINPEER				"adminPeer"
#define REG_ATTR_NODE_DEFAULTPEER			"defaultPeer"
#define REG_ELEMENT_GEOLOCATION_ATTR_LAT		"lat"
#define REG_ELEMENT_GEOLOCATION_ATTR_LON		"lon"
#define REG_ELEMENT_GEOLOCATION_ATTR_CITY		"city"
#define REG_ELEMENT_CONFIGURATION_ATTR_TYPE		"serviceType"
#define REG_ELEMENT_CONFIGURATION_ATTR_PORT		"port"
#define REG_ELEMENT_NODEMANAGER_ATTR_ENDPOINT		"endpoint"

// Get list of PROJECTS
#define QUERY_GET_LIST_OF_PROJECTS  "SELECT name,id from esgf_dashboard.project_dash;"
// Get list of SERVICES 
#define QUERY_GET_LIST_OF_SERVICES  "select (CAST(idhost as varchar) || ':' || CAST(port as varchar)) as key,id from esgf_dashboard.service_instance;"
// Get list of HOST
#define QUERY_GET_LIST_OF_HOSTS  "SELECT ip,id from esgf_dashboard.host;"
// Get list of "USES" 
#define QUERY_GET_LIST_OF_USES  "SELECT (CAST(idproject as varchar ) || ':' || CAST(idserviceinstance as varchar)) as key, idproject from esgf_dashboard.uses;"

// QUERY_INSERT_PROJECT adds a new project (peer group) in the database
#define QUERY_INSERT_PROJECT  "INSERT into esgf_dashboard.project_dash(name,description) values('%s','%s');"

// QUERY QUERY_UPDATE_GEOLOCATION_INFO update geolocation info to an existing host (P2P node) in the database
#define QUERY_UPDATE_GEOLOCATION_INFO  "UPDATE esgf_dashboard.host set city='%s',latitude=%f,longitude=%f where id=%ld;"

// QUERY QUERY_INSERT_HOST adds a new host (P2P node) in the database
#define QUERY_INSERT_NEW_HOST  "INSERT into esgf_dashboard.host(name,ip) values('%s','%s');"

// QUERY QUERY_GET_PROJECT_ID retrieves the id value of a specific project (peer group) 
#define QUERY_GET_PROJECT_ID "SELECT id from esgf_dashboard.project_dash where name='%s';"

// QUERY QUERY_GET_HOST_ID retrieves the id value of a specific host (P2P node) 
#define QUERY_GET_HOST_ID "SELECT id from esgf_dashboard.host where ip='%s';"

// QUERY QUERY_GET_SERVICE_ID retrieves the id value of a specific service 
#define QUERY_GET_SERVICE_ID "SELECT id from esgf_dashboard.service_instance where port=%ld and idhost=%ld;"

// QUERY QUERY_PROJECT_AUTHORIZATION, authorize project (peer group) on guest user in the database
#define QUERY_PROJECT_AUTHORIZATION  "INSERT into esgf_dashboard.join1(iduser,idproject) values(1,%ld);"

// QUERY QUERY_INSERT_SERVICE_INFO, add a service instance in the database
#define QUERY_INSERT_SERVICE_INFO  "INSERT into esgf_dashboard.service_instance(port,name,institution,mail_admin,idhost) values(%ld,'%s','%s','%s',%ld);"

// QUERY QUERY_INSERT_SERVICE_TO_PROJECT binds a service to a project (peer group) in the database
#define QUERY_INSERT_SERVICE_TO_PROJECT  "INSERT into esgf_dashboard.uses(idproject,idserviceinstance) values(%ld,%ld);"

/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o esgf-dashboard-parser esgf-dashboard-parser.c
 */


/* 
The get_foreign_key_value function returns:
-1 = SELECT issue
-2 = more than 1 record (UNIQUE constraint issue) 
>0 = fk value
*/

long int
get_foreign_key_value (PGconn * conn, char *query)
{
  PGresult *res;
  long int fk;
  fprintf (stderr, "Query: %s\n", query);

  res = PQexec (conn, query);
  if ((!res) || (PQresultStatus (res) != PGRES_TUPLES_OK))
    {
      fprintf (stderr, "SELECT command did not return tuples properly\n");
      PQclear (res);
      return -1;
    }

  if ((PQntuples (res)) != 1)
    {
      PQclear (res);
      return -2;
    }

  fk = atol (PQgetvalue (res, 0, 0));

  PQclear (res);
  fprintf (stderr, "The fk is %ld\n", fk);
  return fk;
}



int
submit_query (PGconn * conn, char *query)
{
  PGresult *res;
  fprintf (stderr, "Query: %s\n", query);

  res = PQexec (conn, query);

  if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    {
      fprintf (stderr, "Insert query failed\n");
      PQclear (res);
      return -1;
    }
  PQclear (res);
  return 0;
}


int
htable_test ()
{
  HASHTBL *hashtbl;
  char *spain, *italy;

  if (!(hashtbl = hashtbl_create (16, NULL)))
    {
      fprintf (stderr, "ERROR: hashtbl_create() failed\n");
      exit (EXIT_FAILURE);
    }

  hashtbl_insert (hashtbl, "Italy", "111");
  hashtbl_insert (hashtbl, "Spain", "444");

  printf ("After insert:\n");
  italy = hashtbl_get (hashtbl, "Italy");
  printf ("Italy: %s\n", italy ? italy : "-");
  spain = hashtbl_get (hashtbl, "Spain");
  printf ("Spain: %s\n", spain ? spain : "-");

  hashtbl_remove (hashtbl, "Spain");

  printf ("After remove:\n");
  spain = hashtbl_get (hashtbl, "Spain");
  printf ("Spain: %s\n", spain ? spain : "-");

  hashtbl_resize (hashtbl, 8);

  printf ("After resize:\n");
  italy = hashtbl_get (hashtbl, "Italy");
  printf ("Italy: %s\n", italy ? italy : "-");

  hashtbl_destroy (hashtbl);

  return 0;
}

int
populate_hash_table (PGconn * conn, char *query, HASHTBL ** pointer)
{

  PGresult *res;
  int i, num_records;


  fprintf (stderr, "Retrieving data for hashtable [QUERY=%s]\n", query);
  res = PQexec (conn, query);
  if ((!res) || (PQresultStatus (res) != PGRES_TUPLES_OK))
    {
      fprintf (stderr, "Error running query [%s]\n", query);
      PQclear (res);
      return -1;
    }

  num_records = PQntuples (res);


  for (i = 0; i < num_records; i++)
    {
      hashtbl_insert (*pointer, (char *) PQgetvalue (res, i, 0),
		      (char *) PQgetvalue (res, i, 1));
      fprintf (stderr, "To be stored into the hashtable [%s]#[%s]\n",
	       (char *) PQgetvalue (res, i, 0), (char *) PQgetvalue (res, i,
								     1));
    }
  PQclear (res);

  fprintf (stderr, "Hashtable properly created!\n", query);
  return 0;
}


int
parse_registration_xml_file (xmlNode * a_node)
{
  xmlNode *cur_node = NULL;
  xmlNode *node_node = NULL;
  xmlNode *int_node = NULL;
  xmlNode *gridftp_node = NULL;
  int iter = 0;
  int iter2 = 0;
  int iter3 = 0;
  char *position;
  char *cursor_buf;
  char buffer[2048] = { '\0' };
  char query[2048] = { '\0' };
  PGconn *conn;
  PGresult *res;
  char conninfo[1024] = { '\0' };
  HASHTBL *hashtbl_projects;
  HASHTBL *hashtbl_hosts;
  HASHTBL *hashtbl_services;
  HASHTBL *hashtbl_uses;
  char *hashtbl_result;
  static long int success_lookup[2] = { 0 };	// [0] success [1] missing

  snprintf (conninfo, sizeof (conninfo),
	    "host=%s port=%d dbname=%s user=%s password=%s", POSTGRES_HOST,
	    POSTGRES_PORT_NUMBER, POSTGRES_DB_NAME, POSTGRES_USER,
	    POSTGRES_PASSWD);

  fprintf (stderr, "Open connection to: %s\n", conninfo);

  conn = PQconnectdb ((const char *) conninfo);
  if (PQstatus (conn) != CONNECTION_OK)
    {
      fprintf (stderr, "Connection to database failed: %s",
	       PQerrorMessage (conn));
      PQfinish (conn);
      return -1;
    }

  // Hash tables creation
  fprintf (stderr, "Creating the hashtable for PROJECTS\n");
  if (!(hashtbl_projects = hashtbl_create (16, NULL)))
    {
      fprintf (stderr,
	       "ERROR: hashtbl_create() failed for PROJECTS [skip parsing]\n");
      PQfinish (conn);
      return -2;
    }

  fprintf (stderr, "Creating the hashtable for HOST\n");
  if (!(hashtbl_hosts = hashtbl_create (16, NULL)))
    {
      fprintf (stderr,
	       "ERROR: hashtbl_create() failed for HOST [skip parsing]\n");
      hashtbl_destroy (hashtbl_projects);
      PQfinish (conn);
      return -2;
    }

  fprintf (stderr, "Creating the hashtable for SERVICES\n");
  if (!(hashtbl_services = hashtbl_create (16, NULL)))
    {
      fprintf (stderr,
	       "ERROR: hashtbl_create() failed for SERVICES [skip parsing]\n");
      hashtbl_destroy (hashtbl_projects);
      hashtbl_destroy (hashtbl_hosts);
      PQfinish (conn);
      return -2;
    }

  fprintf (stderr, "Creating the hashtable for USES\n");
  if (!(hashtbl_uses = hashtbl_create (16, NULL)))
    {
      fprintf (stderr,
	       "ERROR: hashtbl_create() failed for USES [skip parsing]\n");
      hashtbl_destroy (hashtbl_projects);
      hashtbl_destroy (hashtbl_hosts);
      hashtbl_destroy (hashtbl_services);
      PQfinish (conn);
      return -2;
    }

  populate_hash_table (conn, QUERY_GET_LIST_OF_PROJECTS, &hashtbl_projects);
  populate_hash_table (conn, QUERY_GET_LIST_OF_HOSTS, &hashtbl_hosts);
  populate_hash_table (conn, QUERY_GET_LIST_OF_SERVICES, &hashtbl_services);
  populate_hash_table (conn, QUERY_GET_LIST_OF_USES, &hashtbl_uses);

  // "Registration" iteration 
  for (cur_node = a_node; cur_node; cur_node = cur_node->next)	// loop on REGISTRATION elements
    {
      if (!strcmp (cur_node->name, "text"))
	continue;
      //printf ("Loop on REGISTRATION: %d\n", ++iter);

      if (cur_node->type == XML_ELEMENT_NODE
	  && (!strcmp (cur_node->name, REG_ELEMENT_REGISTRATION)))
	{
	  fprintf (stderr, "Element->name: %s\n", cur_node->name);
	  // to be done: check on the REGISTRATION timestamp    

	  // test and extract some data
	  hashtbl_result = hashtbl_get (hashtbl_projects, "esgf");
	  fprintf (stderr, "The key related to this project is %s\n",
		   hashtbl_result);
	  hashtbl_result = hashtbl_get (hashtbl_projects, "cssef1");
	  fprintf (stderr, "The key related to this project is %s\n",
		   hashtbl_result);


	  // Loop on NODE elements

	  for (node_node = cur_node->children; node_node; node_node = node_node->next)	// loop on NODE elements
	    {
	      char *organization;
	      char *support_email;
	      char *npg_project;
	      char *node_ip;
	      char *node_hostname;
	      int number_of_projects;
	      long int project_ids[1024] = { 0 };	// this should be dynamically allocated starting from the total number of projects in the hashtable
	      long int service_ids[1024] = { 0 };	// this should be dynamically allocated starting from the total number of services in the hashtable
	      long int host_id = 0;
	      int geolocation_found;
	      int i;

	      if (!strcmp (node_node->name, "text"))
		continue;

	      if (node_node->type == XML_ELEMENT_NODE && (!strcmp (node_node->name, REG_ELEMENT_NODE)))	// if a NODE element 
		{
		  char insert_new_host_query[2048] = { '\0' };
		  char select_id_host_query[2048] = { '\0' };
		  // get NODE attributes values

		  organization =
		    xmlGetProp (node_node, REG_ATTR_NODE_ORGANIZATION);

		  if (organization == NULL || !strcmp (organization, ""))
		    {
		      fprintf (stderr,
			       "Missing/invalid %s [skip current NODE element]\n",
			       REG_ATTR_NODE_ORGANIZATION);
		      xmlFree (organization);
		      continue;
		    }

		  npg_project =
		    xmlGetProp (node_node, REG_ATTR_NODE_NODEPEERGROUP);

		  if (npg_project == NULL || !strcmp (npg_project, ""))
		    {
		      fprintf (stderr,
			       "Missing/invalid %s [skip current NODE element]\n",
			       REG_ATTR_NODE_NODEPEERGROUP);
		      xmlFree (organization);
		      xmlFree (npg_project);
		      continue;
		    }

		  node_ip = xmlGetProp (node_node, REG_ATTR_NODE_NODEIP);

		  if (node_ip == NULL || !strcmp (node_ip, ""))
		    {
		      fprintf (stderr,
			       "Missing/invalid %s [skip current NODE element]\n",
			       REG_ATTR_NODE_NODEIP);
		      xmlFree (organization);
		      xmlFree (npg_project);
		      xmlFree (node_ip);
		      continue;
		    }

		  node_hostname =
		    xmlGetProp (node_node, REG_ATTR_NODE_NODEHOSTNAME);

		  if (node_hostname == NULL || !strcmp (node_hostname, ""))
		    {
		      fprintf (stderr,
			       "Missing/invalid %s [skip current NODE element]\n",
			       REG_ATTR_NODE_NODEHOSTNAME);
		      xmlFree (organization);
		      xmlFree (npg_project);
		      xmlFree (node_ip);
		      xmlFree (node_hostname);
		      continue;
		    }

		  support_email =
		    xmlGetProp (node_node, REG_ATTR_NODE_SUPPORTEMAIL);

		  if (support_email == NULL || !strcmp (support_email, ""))
		    fprintf (stderr,
			     "Missing/invalid %s [No problem... the attribute is not mandatory]\n",
			     REG_ATTR_NODE_SUPPORTEMAIL);

		  // print attributes values
		  fprintf (stderr, "Organization attribute: %s\n",
			   organization);
		  fprintf (stderr, "PeerGroups attribute: %s\n", npg_project);
		  fprintf (stderr, "Hostname attribute: %s\n", node_hostname);
		  fprintf (stderr, "IP attribute: %s\n", node_ip);


		  if (hashtbl_result = hashtbl_get (hashtbl_hosts, node_ip))
		    {
		      fprintf (stderr, "Lookup HostTable hit! [%s] [%s]\n",
			       node_ip, hashtbl_result);
		      host_id = atol (hashtbl_result);
		      success_lookup[0]++;
		    }
		  else
		    {		// add host entry in DB (and hashtable too) without geolocation information

		      char host_id_str[128] = { '\0' };
		      success_lookup[1]++;
		      snprintf (insert_new_host_query,
				sizeof (insert_new_host_query),
				QUERY_INSERT_NEW_HOST, node_hostname,
				node_ip);
		      submit_query (conn, insert_new_host_query);

		      snprintf (select_id_host_query,
				sizeof (select_id_host_query),
				QUERY_GET_HOST_ID, node_ip);
		      host_id =
			get_foreign_key_value (conn, select_id_host_query);
		      // add entry to hash table
		      sprintf (host_id_str, "%ld", host_id);
		      hashtbl_insert (hashtbl_hosts, node_ip, host_id_str);
		      fprintf (stderr,
			       "[LookupFailed] Adding new entry in the hashtable [%s] [%s]\n",
			       node_ip, host_id_str);
		    }

		  fprintf (stderr, "Host %s | id : %ld\n", node_ip, host_id);

		  // isolate PeerGroups adding them to the DB
		  sprintf (buffer, "%s", npg_project);
		  position = &buffer[0];
		  number_of_projects = 0;
		  do
		    {
		      char insert_query[2048] = { '\0' };
		      char select_query[2048] = { '\0' };
		      char authorization_query[2048] = { '\0' };
		      cursor_buf = position;
		      position = strchr (cursor_buf, ',');
		      if (position)
			{
			  *position = '\0';	// now cursor_buf points to the isolated PeerGroup
			  position++;	// position points to the new string (if any)   
			}
		      fprintf (stderr, "PeerGroup %s\n", cursor_buf);

		      if (hashtbl_result =
			  hashtbl_get (hashtbl_projects, cursor_buf))
			{
			  fprintf (stderr,
				   "Lookup ProjectTable hit! [%s] [%s]\n",
				   cursor_buf, hashtbl_result);
			  success_lookup[0]++;
			  project_ids[number_of_projects++] =
			    atol (hashtbl_result);
			}
		      else
			{	// add host entry in DB (and hashtable too) without geolocation information

			  char project_id_str[128] = { '\0' };
			  success_lookup[1]++;
			  // add PeerGroup to project_dash table
			  snprintf (insert_query, sizeof (insert_query),
				    QUERY_INSERT_PROJECT, cursor_buf,
				    cursor_buf);
			  submit_query (conn, insert_query);

			  // get project ID (servira' successivamente)
			  snprintf (select_query, sizeof (select_query),
				    QUERY_GET_PROJECT_ID, cursor_buf);
			  project_ids[number_of_projects++] =
			    get_foreign_key_value (conn, select_query);

			  // added new project into the hashtable
			  sprintf (project_id_str, "%ld",
				   project_ids[number_of_projects - 1]);
			  hashtbl_insert (hashtbl_projects, cursor_buf,
					  project_id_str);
			  fprintf (stderr,
				   "[LookupFailed] Adding new entry in the hashtable [%s] [%s]\n",
				   cursor_buf, project_id_str);

			  // add user to peer group (authorization) 
			  snprintf (authorization_query,
				    sizeof (authorization_query),
				    QUERY_PROJECT_AUTHORIZATION,
				    project_ids[number_of_projects - 1]);
			  submit_query (conn, authorization_query);

			}
		    }
		  while (position);

		  for (i = 0; i < number_of_projects; i++)
		    fprintf (stderr, "Valore %d %ld\n", i, project_ids[i]);

		  fprintf (stderr, "Element->name: %s\n", node_node->name);

		  geolocation_found = 0;

		  // loop on internal NODE elements (CA, GeoLocation, GridFTPService, etc.)
		  for (int_node = node_node->children; int_node;
		       int_node = int_node->next)
		    {
		      if (!strcmp (int_node->name, "text"))
			continue;

		      //printf ("Loop on INTERNALNODE: %d\n", ++iter3);

		      if (int_node->type == XML_ELEMENT_NODE)	// Internal switch 
			{
			  // Start of "if internal node is GeoLocation ELEMENT"
			  if (!strcmp
			      (int_node->name, REG_ELEMENT_GEOLOCATION))
			    {
			      char update_query[2048] = { '\0' };
			      char *latitude;
			      char *longitude;
			      char *city;

			      geolocation_found = 1;
			      latitude =
				xmlGetProp (int_node,
					    REG_ELEMENT_GEOLOCATION_ATTR_LAT);
			      longitude =
				xmlGetProp (int_node,
					    REG_ELEMENT_GEOLOCATION_ATTR_LON);
			      city =
				xmlGetProp (int_node,
					    REG_ELEMENT_GEOLOCATION_ATTR_CITY);

			      // if <GeoLocation> is incomplete or corrupted...
			      if (latitude == NULL || !strcmp (latitude, "")
				  || longitude == NULL
				  || !strcmp (longitude, "") || city == NULL
				  || !strcmp (city, ""))
				{
				  geolocation_found = 0;
				  fprintf (stderr,
					   "The <GeoLocation> element seems to be corrupted or not complete\n");
				}
			      else
				{
				  snprintf (update_query,
					    sizeof (update_query),
					    QUERY_UPDATE_GEOLOCATION_INFO,
					    city, atof (latitude),
					    atof (longitude), host_id);
				  submit_query (conn, update_query);
				}

			      // free XML GEOLOCATION attributes      
			      xmlFree (latitude);
			      xmlFree (longitude);
			      xmlFree (city);
			    }	// end of "if internal node is GeoLocation ELEMENT"

			  // Start of "if internal node is NodeManager ELEMENT"
			  if (!strcmp
			      (int_node->name, REG_ELEMENT_NODEMANAGER))
			    {
			      char insert_query[2048] = { '\0' };
			      char *endpoint;
			      char *slashcursor;
			      char *startcursor;
			      char buffer_endpoint[2048] = { '\0' };
			      long int app_server_port = 80;
			      int iteration = 0;
			      int service_id;
			      char insert_service_query[2048] = { '\0' };
			      char select_id_service_query[2048] = { '\0' };

			      endpoint =
				xmlGetProp (int_node,
					    REG_ELEMENT_NODEMANAGER_ATTR_ENDPOINT);

			      if (endpoint == NULL || !strcmp (endpoint, ""))
				{
				  fprintf (stderr,
					   "Attribute [%s] missing/invalid \n",
					   REG_ELEMENT_NODEMANAGER_ATTR_ENDPOINT);
				  fprintf (stderr,
					   "No %s registered in the database \n",
					   APPLICATION_SERVER_NAME);

				}
			      else
				{	// "if endpoint is valid"                             

				  char service_id_str[128] = { '\0' };
				  char service_key[128] = { '\0' };
				  sprintf (buffer_endpoint, "%s", endpoint);
				  fprintf (stderr, "%s\n", buffer_endpoint);

				  slashcursor = &buffer_endpoint[0];
				  while (slashcursor =
					 strchr (slashcursor, ':'))
				    {
				      iteration++;
				      fprintf (stderr, "Iteration %d\n",
					       iteration);
				      if ((*(++slashcursor)) == '/')
					continue;
				      startcursor = slashcursor;
				      if (slashcursor =
					  strchr (startcursor, '/'))
					{
					  *slashcursor = '\0';
					  app_server_port =
					    atol (startcursor);
					  break;
					}
				    }
				  fprintf (stderr,
					   "%s port: %ld\n",
					   APPLICATION_SERVER_NAME,
					   app_server_port);

				  sprintf (service_key, "%ld:%ld", host_id,
					   app_server_port);
				  if (hashtbl_result =
				      hashtbl_get (hashtbl_services,
						   service_key))
				    {
				      fprintf (stderr,
					       "Lookup HostServices hit! [%s] [%s]\n",
					       service_key, hashtbl_result);
				      service_id = atol (hashtbl_result);
				      success_lookup[0]++;
				    }
				  else
				    {	// add service entry in DB (and hashtable too) without geolocation information
				      // 1) add service
				      char service_id_str[128] = { '\0' };
				      success_lookup[1]++;
				      snprintf (insert_service_query,
						sizeof (insert_service_query),
						QUERY_INSERT_SERVICE_INFO,
						app_server_port,
						APPLICATION_SERVER_NAME,
						organization, support_email,
						host_id);
				      submit_query (conn,
						    insert_service_query);
				      // 2) retrieve service_id
				      // grab service id servizio from port+host
				      snprintf (select_id_service_query,
						sizeof
						(select_id_service_query),
						QUERY_GET_SERVICE_ID,
						app_server_port, host_id);
				      service_id =
					get_foreign_key_value (conn,
							       select_id_service_query);

				      // add new entry into the hashtable
				      sprintf (service_id_str, "%ld",
					       service_id);
				      hashtbl_insert (hashtbl_services,
						      service_key,
						      service_id_str);
				      fprintf (stderr,
					       "[LookupFailed] Adding new entry in the hashtable [%s] [%s]\n",
					       service_key, service_id_str);
				    }
				  fprintf (stderr,
					   "Service %s | id : %ld\n",
					   APPLICATION_SERVER_NAME,
					   service_id);
				  // 3) add service to peer_groups
				  // add services to projects
				  for (i = 0; i < number_of_projects; i++)
				    {
				      char uses_key[128] = { '\0' };
				      sprintf (uses_key, "%ld:%ld",
					       project_ids[i], service_id);
				      if (hashtbl_result =
					  hashtbl_get (hashtbl_uses,
						       uses_key))
					{
					  fprintf (stderr,
						   "Lookup Uses hit! [%s] [%s]\n",
						   uses_key, hashtbl_result);
					  success_lookup[0]++;
					}
				      else
					{
					  char
					    insert_service_2_project_query
					    [2048] = { '\0' };
					  success_lookup[1]++;
					  char project_ids_str[128] =
					    { '\0' };
					  snprintf
					    (insert_service_2_project_query,
					     sizeof
					     (insert_service_2_project_query),
					     QUERY_INSERT_SERVICE_TO_PROJECT,
					     project_ids[i], service_id);
					  submit_query (conn,
							insert_service_2_project_query);
					  // add new entry into the hashtable
					  sprintf (project_ids_str, "%ld",
						   project_ids[i]);
					  hashtbl_insert (hashtbl_uses,
							  uses_key,
							  project_ids_str);
					  fprintf (stderr,
						   "[LookupFailed] Adding new entry in the hashtable [%s] [%s]\n",
						   uses_key, project_ids_str);
					}
				    }	// end for add service to peer groups
				}	// end of "if endpoint is valid
			      // free XML endpoint attribute      
			      xmlFree (endpoint);
			    }	// end of "if internal node is NodeManager ELEMENT"

			  // Start "if internal node is a GridFTPService ELEMENT"       
			  if (!strcmp
			      (int_node->name, REG_ELEMENT_GRIDFTPSERVICE))
			    {

			      // loop on internal GRIFTPSERVICE elements (CONFIGURATION)
			      for (gridftp_node = int_node->children;
				   gridftp_node;
				   gridftp_node = gridftp_node->next)
				{
				  if (!strcmp (gridftp_node->name, "text"))
				    continue;
				  // Start of "if CONFIGURATION element"
				  if (gridftp_node->type == XML_ELEMENT_NODE
				      &&
				      (!strcmp
				       (gridftp_node->name,
					REG_ELEMENT_CONFIGURATION)))
				    {

				      char insert_service_query[2048] =
					{ '\0' };
				      char select_id_service_query[2048] =
					{ '\0' };
				      char *service_type;
				      char *service_port;
				      int service_id;

				      service_type =
					xmlGetProp (gridftp_node,
						    REG_ELEMENT_CONFIGURATION_ATTR_TYPE);
				      service_port =
					xmlGetProp (gridftp_node,
						    REG_ELEMENT_CONFIGURATION_ATTR_PORT);

				      if (service_type == NULL
					  || !strcmp (service_type, "")
					  || service_port == NULL
					  || !strcmp (service_port, ""))
					{
					  fprintf (stderr,
						   "Missing/invalid %s-%s attributes [skip current %s  element]\n",
						   REG_ELEMENT_CONFIGURATION_ATTR_TYPE,
						   REG_ELEMENT_CONFIGURATION_ATTR_PORT,
						   REG_ELEMENT_CONFIGURATION);
					}
				      else
					{	// "if valid serviceType and port"

					  char service_key[128] = { '\0' };

					  sprintf (service_key, "%ld:%ld",
						   host_id,
						   atol (service_port));
					  fprintf (stderr,
						   "Service_key: %s\n",
						   service_key);
					  if (hashtbl_result =
					      hashtbl_get (hashtbl_services,
							   service_key))
					    {
					      fprintf (stderr,
						       "Lookup Services hit! [%s] [%s]\n",
						       service_key,
						       hashtbl_result);
					      service_id =
						atol (hashtbl_result);
					      success_lookup[0]++;
					    }
					  else
					    {	// add service entry in DB (and hashtable too) 
					      // 1) add service
					      char service_id_str[128] =
						{ '\0' };
					      success_lookup[1]++;

					      snprintf (insert_service_query,
							sizeof
							(insert_service_query),
							QUERY_INSERT_SERVICE_INFO,
							atol (service_port),
							service_type,
							organization,
							support_email,
							host_id);
					      submit_query (conn,
							    insert_service_query);

					      // grab service id servizio from port+host
					      snprintf
						(select_id_service_query,
						 sizeof
						 (select_id_service_query),
						 QUERY_GET_SERVICE_ID,
						 atol (service_port),
						 host_id);
					      service_id =
						get_foreign_key_value (conn,
								       select_id_service_query);
					      // add new service into the hashtable
					      sprintf (service_id_str, "%ld",
						       service_id);
					      hashtbl_insert
						(hashtbl_services,
						 service_key, service_id_str);
					      fprintf (stderr,
						       "[LookupFailed] ** Adding new entry in the hashtable [%s] [%s]\n",
						       service_key,
						       service_id_str);
					    }
					  fprintf (stderr,
						   "Service %s | id : %ld\n",
						   service_type, service_id);

					  // add services to projects
					  fprintf (stderr,
						   "ADD SERVICES TO PROJECT: %d\n",
						   number_of_projects);
					  for (i = 0; i < number_of_projects;
					       i++)
					    {
				      		char uses_key[128] = { '\0' };
				      		sprintf (uses_key, "%ld:%ld",project_ids[i], service_id);
				      if (hashtbl_result = hashtbl_get (hashtbl_uses, uses_key))
					{
					  fprintf (stderr,
						   "Lookup Uses hit! [%s] [%s]\n",
						   uses_key, hashtbl_result);
					  success_lookup[0]++;
					}
				      else
					{
					  success_lookup[1]++;
					  char project_ids_str[128] = { '\0' };
					      char
						insert_service_2_project_query
						[2048] = { '\0' };
					      snprintf
						(insert_service_2_project_query,
						 sizeof
						 (insert_service_2_project_query),
						 QUERY_INSERT_SERVICE_TO_PROJECT,
						 project_ids[i], service_id);
					      submit_query (conn,
							    insert_service_2_project_query);

					      fprintf (stderr,
						       "ADD SERVICES TO PROJECT: %s\n",
						       insert_service_2_project_query);
					  // add new entry into the hashtable
					  sprintf (project_ids_str, "%ld",
						   project_ids[i]);
					  hashtbl_insert (hashtbl_uses,
							  uses_key,
							  project_ids_str);
					  fprintf (stderr,
						   "[LookupFailed] Adding new entry in the hashtable [%s] [%s]\n",
						   uses_key, project_ids_str);
						}
					    } // end of for "add services to projects"
					}	// end of "if valid serviceType and port"

				      // free XML CONFIGURATION attributes      
				      xmlFree (service_type);
				      xmlFree (service_port);

				    }	// end of "if CONFIGURATION elements "

				}	// end of loop on internal GRIFTPSERVICE elements (CONFIGURATION, etc.)

			    }	// end of "if internal node is a GridFTPService ELEMENT"

			}	// end of "if internal_node is an ELEMENT node 
		    }		// end of loop on INTERNALNODE elements 
		  if (!geolocation_found)
		    {
		      // adding code here related to GeoIP stuff!!!
		      int code;
		      struct geo_output_struct geo_output;

		      fprintf (stderr,
			       "GeoLocation pieces of info need to be taken from GeoIP library (estimation)\n");

		      if (!esgf_geolookup (node_ip, &geo_output))
			{
			  char update_query[2048] = { '\0' };
			  fprintf (stderr, "[OUTPUT_COUNTRY_CODE=%s]\n",
				   geo_output.country_code);
			  fprintf (stderr, "[OUTPUT_REGION=%s]\n",
				   geo_output.region);
			  fprintf (stderr, "[OUTPUT_CITY=%s]\n",
				   geo_output.city);
			  fprintf (stderr, "[OUTPUT_POSTAL_CODE=%s]\n",
				   geo_output.postal_code);
			  fprintf (stderr, "[OUTPUT_LATITUDE=%f]\n",
				   geo_output.latitude);
			  fprintf (stderr, "[OUTPUT_LONGITUDE=%f]\n",
				   geo_output.longitude);
			  fprintf (stderr, "[OUTPUT_METROCODE=%d]\n",
				   geo_output.metro_code);
			  fprintf (stderr, "[OUTPUT_AREACODE=%d]\n",
				   geo_output.area_code);
			  snprintf (update_query, sizeof (update_query),
				    QUERY_UPDATE_GEOLOCATION_INFO,
				    geo_output.city, geo_output.latitude,
				    geo_output.longitude, host_id);
			  fprintf (stderr,
				   "I got the geolocation info from the DB (estimation)\n%s\n",
				   update_query);
			  submit_query (conn, update_query);

			}
		      else
			fprintf (stderr, "Esgf-lookup error\n");

		    }		// end of "if !geolocation_found" 
		}		// end of "if a NODE element"

	      // free XML NODE attributes
	      xmlFree (organization);
	      xmlFree (npg_project);
	      xmlFree (node_ip);
	      xmlFree (support_email);
	      xmlFree (node_hostname);
	    }			// end of loop on NODE element

	}			// end of "if a REGISTRATION element
    }				// end of loop on REGISTRATION element

  // releasing memory for hashtables
  hashtbl_destroy (hashtbl_projects);
  hashtbl_destroy (hashtbl_hosts);
  hashtbl_destroy (hashtbl_services);
  hashtbl_destroy (hashtbl_uses);

  fprintf(stderr,"*********** Hits %ld Failure %ld ************\n",success_lookup[0],success_lookup[1]);
  // closing database connection
  PQfinish (conn);

  return 0;
}


int
automatic_registration_xml_feed (char *registration_xml_file)
{
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  int result;

  /*parse the file and get the DOM */
  doc = xmlReadFile (registration_xml_file, NULL, 0);

  if (doc == NULL)
    {
      fprintf (stderr, "error: could not parse file %s\n",
	       registration_xml_file);
      return -1;		// error parsing the file       
    }

  /*Get the root element node */
  root_element = xmlDocGetRootElement (doc);

  result = parse_registration_xml_file (root_element);

  /*free the document */
  xmlFreeDoc (doc);

  /*
   *Free the global variables that may
   *have been allocated by the parser.
   */
  xmlCleanupParser ();

  if (result)
    return -2;			// parser failed to connect to the database
  return 0;			// ok
}

/*int
main (int argc, char **argv)
{

  if (argc != 2)
    return (1);
*/
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
/*  LIBXML_TEST_VERSION 
  
  fprintf (stderr, "return code %d\n", automatic_registration_xml_feed (argv[1]));

  return 0;
}*/
