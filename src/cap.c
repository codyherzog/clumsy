// bandwidth cap
#include <math.h>
#include "iup.h"
#include "common.h"
#define NAME "cap"
#define CAP_MIN_KBPS "0"
#define CAP_MAX_KBPS "100000"
#define KEEP_AT_MOST 5000
#define BUFFER_MAX_MS "5000"
#define BUFFER_MIN_MS "0"

#define ALLOW_SET_BUFFER_SIZE 1

static Ihandle *inboundCheckbox, *outboundCheckbox, *kbpsInput, *kbBufMsInput;

static volatile short capEnabled = 0,
    capInbound = 1, capOutbound = 1;
static volatile long
    kbps = 300,
	kbBufMs = 500;

static PacketNode capHeadNode = {0}, capTailNode = {0};
static PacketNode *bufHead = &capHeadNode, *bufTail = &capTailNode;
static int bufSize = 0;
static int bufSizeBytes = 0;

static Ihandle* capSetupUI() {
    Ihandle *capControlsBox = IupHbox(
        inboundCheckbox = IupToggle("Inbound", NULL),
        outboundCheckbox = IupToggle("Outbound", NULL),
        IupLabel("Bandwidth Cap(kbps):"),
        kbpsInput = IupText(NULL),
#if ALLOW_SET_BUFFER_SIZE
        IupLabel("Buffer(ms):"),
		kbBufMsInput = IupText(NULL),
#endif
        NULL
        );

    IupSetCallback(inboundCheckbox, "ACTION", (Icallback)uiSyncToggle);
    IupSetAttribute(inboundCheckbox, SYNCED_VALUE, (char*)&capInbound);
    IupSetCallback(outboundCheckbox, "ACTION", (Icallback)uiSyncToggle);
    IupSetAttribute(outboundCheckbox, SYNCED_VALUE, (char*)&capOutbound);

    IupSetAttribute(kbpsInput, "VISIBLECOLUMNS", "4");
    IupSetAttribute(kbpsInput, "VALUE", "300");
    IupSetCallback(kbpsInput, "VALUECHANGED_CB", (Icallback)uiSyncInteger);
    IupSetAttribute(kbpsInput, SYNCED_VALUE, (char*)&kbps);
    IupSetAttribute(kbpsInput, INTEGER_MAX, CAP_MAX_KBPS);
    IupSetAttribute(kbpsInput, INTEGER_MIN, CAP_MIN_KBPS);

#if ALLOW_SET_BUFFER_SIZE
    IupSetAttribute(kbBufMsInput, "VISIBLECOLUMNS", "4");
    IupSetAttribute(kbBufMsInput, "VALUE", "500");
    IupSetCallback(kbBufMsInput, "VALUECHANGED_CB", uiSyncInteger);
    IupSetAttribute(kbBufMsInput, SYNCED_VALUE, (char*)&kbBufMs);
    IupSetAttribute(kbBufMsInput, INTEGER_MAX, BUFFER_MAX_MS);
    IupSetAttribute(kbBufMsInput, INTEGER_MIN, BUFFER_MIN_MS);
#endif

    // enable by default to avoid confusing
    IupSetAttribute(inboundCheckbox, "VALUE", "ON");
    IupSetAttribute(outboundCheckbox, "VALUE", "ON");

    if (parameterized) {
        setFromParameter(inboundCheckbox, "VALUE", NAME"-inbound");
        setFromParameter(outboundCheckbox, "VALUE", NAME"-outbound");
        setFromParameter(kbpsInput, "VALUE", NAME"-kbps");
#if ALLOW_SET_BUFFER_SIZE
		setFromParameter(kbBufMsInput, "VALUE", NAME"-buf-ms");
#endif
    }

    return capControlsBox;
}

// TODO these are exactly the same as throttle ones, try move them into packet.c
static INLINE_FUNCTION short isBufEmpty() {
    short ret = bufHead->next == bufTail;
    if (ret) assert(bufSize == 0 && bufSizeBytes == 0);
    return ret;
}

static void clearBufPackets(PacketNode *tail) {
    while (!isBufEmpty()) {
		PacketNode* pac = bufHead->next;
		insertBefore(popNode(pac), tail);
		--bufSize;
		bufSizeBytes -= pac->packetLen;
    }
}

static const DWORD timeWindowWidthMs = 20;
static DWORD timeWindowStartMs = 0;
static DWORD timeNextSend = 0;
static int bytesSentInWindow = 0;

static void capStartUp() {
    if (bufHead->next == NULL && bufTail->next == NULL) {
        bufHead->next = bufTail;
        bufTail->prev = bufHead;
    } else {
        assert(isBufEmpty());
    }

    startTimePeriod();
	bufSize = 0;
	bufSizeBytes = 0;
	DWORD curTime = timeGetTime();
	timeWindowStartMs = curTime;
	timeNextSend = curTime;
	bytesSentInWindow = 0;
}

static void capCloseDown(PacketNode *head, PacketNode *tail) {
    UNREFERENCED_PARAMETER(head);

    clearBufPackets(tail);
    endTimePeriod();
}

static short capProcess(PacketNode* head, PacketNode* tail) {
	PacketNode *pac, *pacTmp;

	// Process all live packets. Buffer as many as possible. Drop if buffer is full.
	pac = head->next;
	while (pac != tail) {
		if (!checkDirection(pac->addr.Outbound, capInbound, capOutbound)) {
			pac = pac->next;
			continue;
		}

		const unsigned int maxBufSize = (kbBufMs * kbps) / 8;
		if (bufSize < KEEP_AT_MOST && bufSizeBytes + pac->packetLen <= maxBufSize) {
			pacTmp = pac->next;
			insertBefore(popNode(pac), bufTail);
			++bufSize;
			bufSizeBytes += pac->packetLen;
			pac = pacTmp;
		}
		else
		{
			pacTmp = pac->next;
			popNode(pac);
			freeNode(pac);
			pac = pacTmp;
		}
	}

	if (bufHead->next == bufTail) {
		return 0;
	}

	const int bytesAllowedPerWindow = (timeWindowWidthMs * kbps) / 8;

	DWORD curTime = timeGetTime();
	if ((curTime - timeWindowStartMs) < timeWindowWidthMs) {
		while (bufHead->next != bufTail && bytesSentInWindow < bytesAllowedPerWindow) {
			pac = bufHead->next;
			insertBefore(popNode(pac), tail);
			--bufSize;
			bufSizeBytes -= pac->packetLen;
			bytesSentInWindow += pac->packetLen;
		}
	}
	else {
		timeWindowStartMs += timeWindowWidthMs;

		if (bytesSentInWindow > bytesAllowedPerWindow) {
			bytesSentInWindow -= bytesAllowedPerWindow;
		} else {
			bytesSentInWindow = 0;
		}
	}

	return 1;
}

Module capModule = {
    "Cap",
    NAME,
    (short*)&capEnabled,
    capSetupUI,
    capStartUp,
    capCloseDown,
    capProcess,
    // runtime fields
    0, 0, NULL
};