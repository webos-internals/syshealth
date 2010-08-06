/*=============================================================================
 Copyright (C) 2010 WebOS Internals <support@webos-internals.org>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "luna_service.h"
#include "luna_methods.h"

#define ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"

//
// We use static buffers instead of continually allocating and deallocating stuff,
// since we're a long-running service, and do not want to leak anything.
//
static char buffer[MAXBUFLEN];
static char esc_buffer[MAXBUFLEN];
static char run_command_buffer[MAXBUFLEN];

//
// Escape a string so that it can be used directly in a JSON response.
// In general, this means escaping quotes, backslashes and control chars.
// It uses the static esc_buffer, which must be twice as large as the
// largest string this routine can handle.
//
static char *json_escape_str(char *str)
{
  const char *json_hex_chars = "0123456789abcdef";

  // Initialise the output buffer
  strcpy(esc_buffer, "");

  // Check the constraints on the input string
  if (strlen(str) > MAXBUFLEN) return (char *)esc_buffer;

  // Initialise the pointers used to step through the input and output.
  char *resultsPt = (char *)esc_buffer;
  int pos = 0, start_offset = 0;

  // Traverse the input, copying to the output in the largest chunks
  // possible, escaping characters as we go.
  unsigned char c;
  do {
    c = str[pos];
    switch (c) {
    case '\0':
      // Terminate the copying
      break;
    case '\b':
    case '\n':
    case '\r':
    case '\t':
    case '"':
    case '\\': {
      // Copy the chunk before the character which must be escaped
      if (pos - start_offset > 0) {
	memcpy(resultsPt, str + start_offset, pos - start_offset);
	resultsPt += pos - start_offset;
      };
      
      // Escape the character
      if      (c == '\b') {memcpy(resultsPt, "\\b",  2); resultsPt += 2;} 
      else if (c == '\n') {memcpy(resultsPt, "\\n",  2); resultsPt += 2;} 
      else if (c == '\r') {memcpy(resultsPt, "\\r",  2); resultsPt += 2;} 
      else if (c == '\t') {memcpy(resultsPt, "\\t",  2); resultsPt += 2;} 
      else if (c == '"')  {memcpy(resultsPt, "\\\"", 2); resultsPt += 2;} 
      else if (c == '\\') {memcpy(resultsPt, "\\\\", 2); resultsPt += 2;} 

      // Reset the start of the next chunk
      start_offset = ++pos;
      break;
    }

    default:
      
      // Check for "special" characters
      if ((c < ' ') || (c > 127)) {

	// Copy the chunk before the character which must be escaped
	if (pos - start_offset > 0) {
	  memcpy(resultsPt, str + start_offset, pos - start_offset);
	  resultsPt += pos - start_offset;
	}

	// Insert a normalised representation
	sprintf(resultsPt, "\\u00%c%c",
		json_hex_chars[c >> 4],
		json_hex_chars[c & 0xf]);

	// Reset the start of the next chunk
	start_offset = ++pos;
      }
      else {
	// Just move along the source string, without copying
	pos++;
      }
    }
  } while (c);

  // Copy the final chunk, if required
  if (pos - start_offset > 0) {
    memcpy(resultsPt, str + start_offset, pos - start_offset);
    resultsPt += pos - start_offset;
  } 

  // Terminate the output buffer ...
  memcpy(resultsPt, "\0", 1);

  // and return a pointer to it.
  return (char *)esc_buffer;
}

//
// A dummy method, useful for unimplemented functions or as a status function.
// Called directly from webOS, and returns directly to webOS.
//
bool dummy_method(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  if (!LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror)) goto error;

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  return false;
}

//
// Run a shell command, and return the output in-line in a buffer for returning to webOS.
// If lshandle, message and subscriber are defined, then also send back status messages.
// The global run_command_buffer must be initialised before calling this function.
// The return value says whether the command executed successfully or not.
//
static bool run_command(char *command, bool escape) {
  LSError lserror;
  LSErrorInit(&lserror);

  // Local buffers to store the current and previous lines.
  char line[MAXLINLEN];

  // fprintf(stderr, "Running command %s\n", command);

  // run_command_buffer is assumed to be initialised, ready for strcat to append.

  // Is this the first line of output?
  bool first = true;

  bool array = false;

  // Start execution of the command, and read the output.
  FILE *fp = popen(command, "r");

  // Return immediately if we cannot even start the command.
  if (!fp) {
    return false;
  }

  // Loop through the output lines
  while (fgets(line, sizeof line, fp)) {

    // Chomp the newline
    char *nl = strchr(line,'\n'); if (nl) *nl = 0;

    // Add formatting breaks between lines
    if (first) {
      if (run_command_buffer[strlen(run_command_buffer)-1] == '[') {
	array = true;
      }
      first = false;
    }
    else {
      if (array) {
	strcat(run_command_buffer, ", ");
      }
      else {
	strcat(run_command_buffer, "<br>");
      }
    }
    
    // Append the unfiltered output to the run_command_buffer.
    if (escape) {
      if (array) {
	strcat(run_command_buffer, "\"");
      }
      strcat(run_command_buffer, json_escape_str(line));
      if (array) {
	strcat(run_command_buffer, "\"");
      }
    }
    else {
      strcat(run_command_buffer, line);
    }
  }
  
  // Check the close status of the process
  if (pclose(fp)) {
    return false;
  }

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  // %%% We need a way to distinguish command failures from LSMessage failures %%%
  // %%% This may need to be true if we just want to ignore LSMessage failures %%%
  return false;
}

//
// Send a standard format command failure message back to webOS.
// The command will be escaped.  The output argument should be a JSON array and is not escaped.
// The additional text  will not be escaped.
// The return value is from the LSMessageReply call, not related to the command execution.
//
static bool report_command_failure(LSHandle* lshandle, LSMessage *message, char *command, char *stdErrText, char *additional) {
  LSError lserror;
  LSErrorInit(&lserror);

  // Include the command that was executed, in escaped form.
  snprintf(buffer, MAXBUFLEN,
	   "{\"errorText\": \"Unable to run command: %s\"",
	   json_escape_str(command));

  // Include any stderr fields from the command.
  if (stdErrText) {
    strcat(buffer, ", \"stdErr\": ");
    strcat(buffer, stdErrText);
  }

  // Report that an error occurred.
  strcat(buffer, ", \"returnValue\": false, \"errorCode\": -1");

  // Add any additional JSON fields.
  if (additional) {
    strcat(buffer, ", ");
    strcat(buffer, additional);
  }

  // Terminate the JSON reply message ...
  strcat(buffer, "}");

  // fprintf(stderr, "Message is %s\n", buffer);

  // and send it.
  if (!LSMessageReply(lshandle, message, buffer, &lserror)) goto error;

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  return false;
}

//
// Run a simple shell command, and return the output to webOS.
//
static bool simple_command(LSHandle* lshandle, LSMessage *message, char *command) {
  LSError lserror;
  LSErrorInit(&lserror);

  // Initialise the output buffer
  strcpy(run_command_buffer, "{\"stdOut\": [");

  // Run the command
  if (run_command(command, true)) {

    // Finalise the message ...
    strcat(run_command_buffer, "], \"returnValue\": true}");

    // fprintf(stderr, "Message is %s\n", run_command_buffer);

    // and send it to webOS.
    if (!LSMessageReply(lshandle, message, run_command_buffer, &lserror)) goto error;
  }
  else {

    // Finalise the command output ...
    strcat(run_command_buffer, "]");

    // and use it in a failure report message.
    if (!report_command_failure(lshandle, message, command, run_command_buffer+11, NULL)) goto end;
  }

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  return false;
}

//
// Read a single string from a file, and return it to webOS.
//
static bool read_single_line(LSHandle* lshandle, LSMessage *message, char *file) {
  LSError lserror;
  LSErrorInit(&lserror);

  char line[MAXLINLEN];

  FILE *fp = fopen(file, "r");

  if (!fp) {
    sprintf(buffer, "{\"errorText\": \"Unable to open %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
  }
  else {
    int value;
    if (fgets(line, MAXLINLEN-1, fp)) {
      sprintf(buffer, "{\"value\": \"%s\", \"returnValue\": true }", json_escape_str(line));
    }
    else {
      sprintf(buffer, "{\"errorText\": \"Unable to parse %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
    }
    if (fclose(fp)) {
      sprintf(buffer, "{\"errorText\": \"Unable to close %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
    }
  }

  fprintf(stderr, "Message is %s\n", buffer);
  if (!LSMessageReply(lshandle, message, buffer, &lserror)) goto error;

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  return false;
}

//
// Read a single integer from a file, and return it to webOS.
//
static bool read_single_integer(LSHandle* lshandle, LSMessage *message, char *file) {
  LSError lserror;
  LSErrorInit(&lserror);

  FILE *fp = fopen(file, "r");

  if (!fp) {
    sprintf(buffer, "{\"errorText\": \"Unable to open %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
  }
  else {
    int value;
    if (fscanf(fp, "%d", &value) == 1) {
      sprintf(buffer, "{\"value\": %d, \"returnValue\": true }", value);
    }
    else {
      sprintf(buffer, "{\"errorText\": \"Unable to parse %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
    }
    if (fclose(fp)) {
      sprintf(buffer, "{\"errorText\": \"Unable to close %s\", \"returnValue\": false, \"errorCode\": -1 }", file);
    }
  }

  // fprintf(stderr, "Message is %s\n", buffer);
  if (!LSMessageReply(lshandle, message, buffer, &lserror)) goto error;

  return true;
 error:
  LSErrorPrint(&lserror, stderr);
  LSErrorFree(&lserror);
 end:
  return false;
}

//
// Read /sys/w1_bus_master1/getvoltage
//
bool get_sys_w1_bus_master1_getvoltage_method(LSHandle* lshandle, LSMessage *message, void *ctx) {
  return simple_command(lshandle, message, "/bin/cat /sys/devices/w1_bus_master1/32*/getvoltage 2>&1");
}

//
// Read /sys/w1_bus_master1/getpercent
//
bool get_sys_w1_bus_master1_getpercent_method(LSHandle* lshandle, LSMessage *message, void *ctx) {
  return simple_command(lshandle, message, "/bin/cat /sys/devices/w1_bus_master1/32*/getpercent 2>&1");
}

//
// Run VACUUM job on SQLite databases
// Probably need file to test if *db.* is a SQLite db
//
bool run_sqlite_vacuum(LSHandle* lshandle, LSMessage *message, void *ctx) {
  return simple_command(lshandle, message, "/var/svc/org.webosinternals.syshealth/sqlite_vacuum.sh 2>&1");
}

//
// Remove email and attachements older than 7 days
// Need a way to pass in an integer to set the number of days. Probably 1-21 days?
// Need a way to pop up a reboot dialog after this runs
//
bool run_email_cleanup(LSHandle* lshandle, LSMessage *message, void *ctx) {
  return simple_command(lshandle, message, "/var/svc/org.webosinternals.syshealth/email_cleanup.sh 2>&1");
}

//
// Flush caches to free up some memory
//
bool run_drop_caches(LSHandle* lshandle, LSMessage *message, void *ctx) {
  return simple_command(lshandle, message, "/var/svc/org.webosinternals.syshealth/email_cleanup.sh 2>&1");
}

LSMethod luna_methods[] = {
  { "status",			dummy_method },
  { "get_w1_voltage",		get_sys_w1_bus_master1_getvoltage_method },
  { "get_w1_percent", 		get_sys_w1_bus_master1_getpercent_method },
  { "sqlite_vacuum", 		run_sqlite_vacuum },
  { "email_cleanup", 		run_email_cleanup },
  { "drop_caches", 		run_drop_caches },
  { 0, 0 }
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
  return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
				       NULL, NULL, NULL, &lserror);
}
