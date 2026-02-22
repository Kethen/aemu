struct game_patch{
	const char *disc_id;
	#ifdef DEBUG
	const char *patch_name;
	#endif
	void *location;
	const uint8_t *patch_content;
	int patch_size;
	void (*custom_patcher)();
};

static const struct game_patch p5_patches[] = {
	{
		.disc_id = "ULJM05800",
		#ifdef DEBUG
		.patch_name = "Monster Hunter Portable 3rd p5 compactor",
		#endif
		.location = (void *)0x088f263c,
		.patch_content = (uint8_t[]){0x26},
		.patch_size = 1,
		.custom_patcher = NULL,
	},
};
