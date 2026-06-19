/* Headless bridge round-trip test: PING the host daemon through the DOSBox
 * filesystem and write the outcome to BRIDGE\TESTOUT.TXT. Verifies the
 * placeholder/read-back path works through the DOSBox directory cache.
 * Build: wcl386 -bt=dos -l=dos4g bridge_test.c -fe=bridge_test.exe */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dos.h>

#define BREQ_TMP "C:\\OPENQT\\BRIDGE\\REQ.TMP"
#define BREQ     "C:\\OPENQT\\BRIDGE\\REQ.TXT"
#define BRESP    "C:\\OPENQT\\BRIDGE\\RESP.TXT"
#define BOUT     "C:\\OPENQT\\BRIDGE\\TESTOUT.TXT"

int main(void)
{
    FILE *fp;
    char resp[1024];
    char *eof, *nl;
    time_t start;
    int n;

    fp = fopen(BRESP, "wb"); if (!fp) { fp = fopen(BOUT, "wb"); fputs("FAIL: cannot write RESP\n", fp); fclose(fp); return 1; }
    fputs("WAIT\n", fp); fclose(fp);

    fp = fopen(BREQ_TMP, "wb"); fprintf(fp, "PING\nEN\n---\nheadless-dosbox-test"); fclose(fp);
    remove(BREQ); rename(BREQ_TMP, BREQ);

    start = time(NULL);
    while (1) {
        fp = fopen(BRESP, "rb");
        if (fp) {
            n = fread(resp, 1, sizeof(resp) - 1, fp); fclose(fp);
            if (n < 0) n = 0; resp[n] = '\0';
            eof = strstr(resp, "\n.EOF.");
            if (eof) {
                *eof = '\0';
                nl = strchr(resp, '\n');
                fp = fopen(BOUT, "wb");
                if (nl && strncmp(resp, "OK", 2) == 0)
                    fprintf(fp, "PASS payload=[%s]\n", nl + 1);
                else
                    fprintf(fp, "FAIL status=[%s]\n", resp);
                fclose(fp);
                return 0;
            }
        }
        if ((long)(time(NULL) - start) > 20) {
            fp = fopen(BOUT, "wb"); fputs("FAIL: timeout (no .EOF.)\n", fp); fclose(fp);
            return 2;
        }
        delay(150);
    }
}
