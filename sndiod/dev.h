/*	$OpenBSD$	*/
/*
 * Copyright (c) 2008-2012 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef DEV_H
#define DEV_H

#include "abuf.h"
#include "dsp.h"
#include "siofile.h"
#include "dev_siomix.h"

#define CTLADDR_SLOT_LEVEL(n)	(n)
#define CTLADDR_MASTER		(DEV_NSLOT)
#define CTLADDR_END		(DEV_NSLOT + 1)

/*
 * audio stream state structure
 */

struct slotops
{
	void (*onmove)(void *);			/* clock tick */
	void (*onvol)(void *);	        /* tell client vol changed */
	void (*fill)(void *);			/* request to fill a play block */
	void (*flush)(void *);			/* request to flush a rec block */
	void (*eof)(void *);			/* notify that play drained */
	void (*exit)(void *);			/* delete client */
};

struct slot {
	struct slotops *ops;			/* client callbacks */
	struct slot *next;			/* next on the play list */
	struct dev *dev;			/* device this belongs to */
	void *arg;				/* user data for callbacks */
	struct aparams par;			/* socket side params */
	struct {
		int weight;			/* dynamic range */
		int maxweight;			/* max dynamic range allowed */
		unsigned int vol;		/* volume within the vol */
		struct abuf buf;		/* socket side buffer */
		int bpf;			/* byte per frame */
		int slot_cmin, slot_cmax;	/* slot source chans */
		int dev_cmin, dev_cmax;		/* device destination chans */
		struct cmap cmap;		/* channel mapper state */
		struct resamp resamp;		/* resampler state */
		struct conv dec;		/* format decoder params */
		int join;			/* channel join factor */
		int expand;			/* channel expand factor */
		void *resampbuf, *decbuf;	/* tmp buffers */
	} mix;
	struct {
		struct abuf buf;		/* socket side buffer */
		int prime;			/* initial cycles to skip */
		int bpf;			/* byte per frame */
		int slot_cmin, slot_cmax;	/* slot destination chans */
		int dev_cmin, dev_cmax;		/* device source chans */
		struct cmap cmap;		/* channel mapper state */
		struct resamp resamp;		/* buffer for resampling */
		struct conv enc;		/* buffer for encoding */
		int join;			/* channel join factor */
		int expand;			/* channel expand factor */
		void *resampbuf, *encbuf;	/* tmp buffers */
	} sub;
	int xrun;				/* underrun policy */
	int skip;				/* cycles to skip (for xrun) */
	int dup;				/* mono-to-stereo and alike */
#define SLOT_BUFSZ(s) \
	((s)->appbufsz + (s)->dev->bufsz / (s)->dev->round * (s)->round)
	int appbufsz;				/* slot-side buffer size */
	int round;				/* slot-side block size */
	int rate;				/* slot-side sample rate */
	int delta;				/* pending clock ticks */
	int delta_rem;				/* remainder for delta */
	int mode;				/* MODE_{PLAY,REC} */
#define SLOT_INIT	0			/* not trying to do anything */
#define SLOT_START	1			/* buffer allocated */
#define SLOT_READY	2			/* buffer filled enough */
#define SLOT_RUN	3			/* buffer attached to device */
#define SLOT_STOP	4			/* draining */
	int pstate;

#define SLOT_NAMEMAX	8
	char name[SLOT_NAMEMAX];		/* name matching [a-z]+ */
	unsigned int unit;			/* instance of name */
	unsigned int serial;			/* global unique number */
	unsigned int vol;			/* current (midi) volume */
	unsigned int tstate;			/* mmc state */
};

/*
 * subset of channels of a stream
 */

struct ctl {
	struct ctl *next;
#define CTL_NUM		2		/* number (aka integer value) */
#define CTL_SW		3		/* on/off switch, only bit 7 counts */
#define CTL_VEC		4		/* number, element of vector */
#define CTL_LIST	5		/* switch, element of a list */
	unsigned int type;		/* one of above */
	unsigned int addr;		/* control address */
#define CTL_NAMEMAX	16		/* max name lenght */
	char func[CTL_NAMEMAX];		/* parameter function name */
	struct ctl_chan {
		char str[CTL_NAMEMAX];	/* stream name */
		int unit;
	} group, chan0, chan1;			/* affected channels */
	unsigned int val_mask;
	unsigned int desc_mask;
	unsigned int curval;
	int dirty;
};

struct ctlslot {
	struct dev *dev;
	unsigned int mask;
	unsigned int mode;
	int inuse;
};

/*
 * audio device with plenty of slots
 */
struct dev {
	struct dev *next;
	struct slot *slot_list;			/* audio streams attached */
	struct midi *midi;

	/*
	 * audio device (while opened)
	 */
	struct dev_sio sio;
	struct dev_siomix siomix;
	struct aparams par;			/* encoding */
	int pchan, rchan;			/* play & rec channels */
	adata_t *rbuf;				/* rec buffer */
	adata_t *pbuf;				/* array of play buffers */
#define DEV_PBUF(d) ((d)->pbuf + (d)->poffs * (d)->pchan)
	int poffs;				/* index of current play buf */
	int psize;				/* size of play buffer */
	struct conv enc;			/* native->device format */
	struct conv dec;			/* device->native format */
	unsigned char *encbuf;			/* buffer for encoding */
	unsigned char *decbuf;			/* buffer for decoding */

	/*
	 * preallocated audio sub-devices
	 */
#define DEV_NSLOT	8
	struct slot slot[DEV_NSLOT];
	unsigned int serial;			/* for slot allocation */

	/*
	 * current position, relative to the current cycle
	 */
	int delta;

	/*
	 * desired parameters
	 */
	unsigned int reqmode;			/* mode */
	struct aparams reqpar;			/* parameters */
	int reqpchan, reqrchan;			/* play & rec chans */
	unsigned int reqbufsz;			/* buffer size */
	unsigned int reqround;			/* block size */
	unsigned int reqrate;			/* sample rate */
	unsigned int hold;			/* hold the device open ? */
	unsigned int autovol;			/* auto adjust playvol ? */
	unsigned int refcnt;			/* number of openers */
#define DEV_NMAX	16			/* max number of devices */
	unsigned int num;			/* device serial number */
#define DEV_CFG		0			/* closed */
#define DEV_INIT	1			/* stopped */
#define DEV_RUN		2			/* playin & recording */
	unsigned int pstate;			/* one of above */
	char *path;				/* sio path */

	/*
	 * actual parameters and runtime state (i.e. once opened)
	 */
	unsigned int mode;			/* bitmap of MODE_xxx */
	unsigned int bufsz, round, rate;
	unsigned int prime;

	/*
	 * MIDI time code (MTC)
	 */
	struct {
		unsigned int origin;		/* MTC start time */
		unsigned int fps;		/* MTC frames per second */
#define MTC_FPS_24	0
#define MTC_FPS_25	1
#define MTC_FPS_30	3
		unsigned int fps_id;		/* one of above */
		unsigned int hr;		/* MTC hours */
		unsigned int min;		/* MTC minutes */
		unsigned int sec;		/* MTC seconds */
		unsigned int fr;		/* MTC frames */
		unsigned int qfr;		/* MTC quarter frames */
		int delta;			/* rel. to the last MTC tick */
		int refs;
	} mtc;

	/*
	 * MIDI machine control (MMC)
	 */
#define MMC_OFF		0			/* ignore MMC messages */
#define MMC_STOP	1			/* stopped, can't start */
#define MMC_START	2			/* attempting to start */
#define MMC_RUN		3			/* started */
	unsigned int tstate;			/* one of above */
	unsigned int master;			/* master volume controller */

	/*
	 * control
	 */

	struct ctl *ctl_list;
#define DEV_NCTLSLOT 8
	struct ctlslot ctlslot[DEV_NCTLSLOT];
};

extern struct dev *dev_list;

void dev_log(struct dev *);
void dev_close(struct dev *);
struct dev *dev_new(char *, struct aparams *, unsigned int, unsigned int,
    unsigned int, unsigned int, unsigned int, unsigned int);
struct dev *dev_bynum(int);
void dev_del(struct dev *);
void dev_adjpar(struct dev *, int, int, int);
int  dev_init(struct dev *);
void dev_done(struct dev *);
int dev_ref(struct dev *);
void dev_unref(struct dev *);
int  dev_getpos(struct dev *);
unsigned int dev_roundof(struct dev *, unsigned int);

/*
 * interface to hardware device
 */
void dev_onmove(struct dev *, int);
void dev_cycle(struct dev *);

/*
 * midi & midi call-backs
 */
void dev_mmcstart(struct dev *);
void dev_mmcstop(struct dev *);
void dev_mmcloc(struct dev *, unsigned int);
void dev_master(struct dev *, unsigned int);
void dev_midi_vol(struct dev *, struct slot *);

/*
 * sio_open(3) like interface for clients
 */
void slot_log(struct slot *);
struct slot *slot_new(struct dev *, char *, struct slotops *, void *, int);
void slot_del(struct slot *);
void slot_setvol(struct slot *, unsigned int);
void slot_start(struct slot *);
void slot_stop(struct slot *);
void slot_read(struct slot *);
void slot_write(struct slot *);

/*
 * control related functions
 */
void ctl_log(struct ctl *);
struct ctlslot *ctlslot_new(struct dev *);
void ctlslot_del(struct ctlslot *);
int dev_setctl(struct dev *, int, int);
int dev_onctl(struct dev *, int, int);
int dev_nctl(struct dev *);
void dev_label(struct dev *, int);
struct ctl *dev_addctl(struct dev *, char *, int, int, int,
    char *, int, char *, char *, int, int);
void dev_rmctl(struct dev *, int);
int dev_makeunit(struct dev *, char *);

#endif /* !defined(DEV_H) */
