/* Copyright (c) 2013 Gianni Tedesco
 * All rights reserved
 * Author: Gianni Tedesco
*/

#define DEBUG_MODULE 1
#include <scaraOS/kernel.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/blk.h>
#include <scaraOS/partition.h>

#define MBR_NUM_PART		 4
#define MBR_SIG_OFFSET		0x1fe
#define MBR_SIG_VALUE		0xaa55
#define MBR_PART_OFFSET		0x1be
#define MBR_SERIAL_OFFSET	0x1b8
#define MBR_PART_SIZE		0x10
#define MBR_CHS_LIMIT		(1024*255*63 - 1)

/* One partition of a disk */
struct _partition {
	struct _ptbl *p_tbl;
	uint8_t p_status;
	uint8_t p_type;
	uint64_t p_lba_start;
	uint64_t p_lba_end;
};

/* A disk with a partition table */
struct _ptbl {
	struct blkdev *pt_blk;
	struct blk_geom pt_geom;
	unsigned int pt_count;
	uint32_t pt_sig;
	struct _partition *pt_part;
};

static unsigned int chs_to_lba(ptbl_t pt, uint8_t *chs)
{
	unsigned int c, h, s, lba;

	c = ((chs[1] & 0xc0) << 2) | chs[2];
	h = chs[0];
	s = chs[1] & 0x3f;

	lba = (c * pt->pt_geom.h + h) * pt->pt_geom.s + s - 1;
	dprintk("chs2lba: %u/%u/%u = %u\n", c, h, s, lba);
	if (lba >= MBR_CHS_LIMIT)
		dprintk("Reached CHS up limit: %u/%u/%u = %u\n", c, h, s, lba);
	return lba;
}

static int read_pt(ptbl_t pt)
{
	uint8_t buf[512];
	unsigned int i, j;
	uint8_t *ptr;
	int rc;

	pt->pt_count = 0;

	/* Pull in MBR */
	rc = 0;//vdisk_read(pt->pt_hdd, 0, buf, sizeof(buf));
	if ( rc ) {
		return rc;
	}

	//hex_dump(buf, sizeof(buf), 0);

	if ( MBR_SIG_VALUE != *(uint16_t *)(buf + MBR_SIG_OFFSET) ) {
		dprintk("bad MBR sig\n");
		return -1;
	}

	pt->pt_sig = *(uint32_t *)(buf + MBR_SERIAL_OFFSET);

	/* Just count */
	for(ptr = buf + MBR_PART_OFFSET, i = 0; i < 4; i++,
			ptr += MBR_PART_SIZE) {
		uint8_t type = ptr[4];
		switch(type) {
		case PART_TYPE_EMPTY:
			continue;
		case PART_TYPE_EXTENDED:
		case PART_TYPE_EXTENDED_LBA:
			printk(" *** Ignoring extended partition %u\n", i);
			continue;
		default:
			pt->pt_count++;
			break;
		}
	}

	pt->pt_part = kmalloc0(pt->pt_count * sizeof(*pt->pt_part));
	if ( NULL == pt->pt_part ) {
		return -1; // ENOMEM
	}

	for(ptr = buf + MBR_PART_OFFSET, i = j = 0; i < 4; i++,
			ptr += MBR_PART_SIZE) {
		uint8_t status, type;

		status = ptr[0];
		type = ptr[4];

		switch(type) {
		case PART_TYPE_EMPTY:
		case PART_TYPE_EXTENDED:
		case PART_TYPE_EXTENDED_LBA:
			continue;
		default:
			break;
		}

		dprintk("%u: %spartition type 0x%.2x\n", j,
			(status == 0x80) ? "bootable " : "",
			type);

		pt->pt_part[j].p_tbl = pt;
		pt->pt_part[j].p_status = status;
		pt->pt_part[j].p_type = type;

		/* CHS has a up limit */
		if (chs_to_lba(pt, ptr + 1) < MBR_CHS_LIMIT &&
			chs_to_lba(pt, ptr + 5) < MBR_CHS_LIMIT) {
			pt->pt_part[j].p_lba_start = chs_to_lba(pt, ptr + 1);
			pt->pt_part[j].p_lba_end = chs_to_lba(pt, ptr + 5);
		} else {
			pt->pt_part[j].p_lba_start =
				le32toh(*(uint32_t *)(ptr + 8));
			pt->pt_part[j].p_lba_end =
				le32toh(*(uint32_t *)(ptr + 8)) +
				le32toh(*(uint32_t *)(ptr + 12));
		}

		if ( pt->pt_part[j].p_lba_start > pt->pt_part[j].p_lba_end ) {
			pt->pt_part[j].p_type = PART_TYPE_EMPTY;
			printk(" *** Bad partition: start > end\n");
		}

		j++;
	}

	return 0;
}

ptbl_t ptbl_open(struct blkdev *blk)
{
	ptbl_t pt;
	int rc;

	pt = kmalloc0(sizeof(*pt));
	if ( NULL == pt ) {
		return pt; /* ENOMEM */
	}

	pt->pt_blk = blk;

	rc = 0;//vdisk_get_geom(pt->pt_hdd, &pt->pt_geom);
	if (rc) {
		/* hopefully sane defaults */
		pt->pt_geom.c = 1024;
		pt->pt_geom.h = 255;
		pt->pt_geom.s = 63;
	}

	dprintk("GEOM: c/h/s = %u/%u/%u\n",
		pt->pt_geom.c,
		pt->pt_geom.h,
		pt->pt_geom.s);

	if ( !read_pt(pt) )
		goto err_free;

	dprintk("Found %d partitions\n", pt->pt_count);
	return pt;
err_free:
	kfree(pt);
	return NULL;
}

int ptbl_set_signature(ptbl_t pt, uint32_t sig)
{
	char buf[512];
	int rc;

	rc = 0;//vdisk_read(pt->pt_hdd, 0, buf, sizeof(buf));
	if (rc) {
		return rc;
	}

	*(uint32_t *)(buf + MBR_SERIAL_OFFSET) = sig;

	rc = 0;//vdisk_write(pt->pt_hdd, 0, buf, sizeof(buf));
	if (rc) {
		return rc;
	}

	pt->pt_sig = sig;
	return 0;
}

uint32_t ptbl_disk_signature(ptbl_t pt)
{
	return pt->pt_sig;
}

unsigned int ptbl_count_partitions(ptbl_t pt)
{
	return pt->pt_count;
}

partition_t ptbl_get_partition(ptbl_t pt, unsigned int idx)
{
	//assert(idx < pt->pt_count);
	return pt->pt_part + idx;
}

void ptbl_close(ptbl_t pt)
{
	if ( pt ) {
		kfree(pt->pt_part);
		kfree(pt);
	}
}

uint8_t part_type(partition_t part)
{
	return part->p_type;
}

uint8_t part_status(partition_t part)
{
	return part->p_status;
}

uint64_t part_num_sectors(partition_t part)
{
	return (part->p_lba_end + 1) - part->p_lba_start;
}

uint64_t part_start_sector(partition_t part)
{
	return part->p_lba_start;
}

int part_read_sectors(partition_t part, void *buf,
			uint64_t sec, unsigned int num_sec)
{
	ptbl_t pt = part->p_tbl;
	uint64_t ofs;
	int rc;

	ofs = (sec + part->p_lba_start);
	//assert(ofs + num_sec < (part->p_lba_end - part->p_lba_start));

	rc = 0;//vdisk_read(pt->pt_hdd, ofs * SECTOR_SIZE, buf, num_sec * SECTOR_SIZE);
	if (rc) {
		return rc;
	}

	return 0;
}

int part_write_sectors(partition_t part, const void *buf,
			uint64_t sec, unsigned int num_sec)
{
	ptbl_t pt = part->p_tbl;
	uint64_t ofs;
	int rc;

	ofs = (sec + part->p_lba_start);
	//assert(ofs + num_sec < (part->p_lba_end - part->p_lba_start));

	rc = 0;//vdisk_write(pt->pt_hdd, ofs * SECTOR_SIZE, buf, num_sec * SECTOR_SIZE);
	if (rc) {
		return rc;
	}

	return 0;
}
