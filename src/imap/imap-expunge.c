/* Copyright (c) 2003-2017 Dovecot authors, see the included COPYING file */

#include "imap-common.h"
#include "mail-storage.h"
#include "mail-search-build.h"
#include "imap-expunge.h"

int imap_expunge(struct mailbox *box, struct mail_search_arg *next_search_arg,
		 unsigned int *expunged_count)
{
	struct mail_search_context *ctx;
        struct mailbox_transaction_context *t;
	struct mail *mail;
	struct mail_search_args *search_args;
	bool expunges = FALSE;

	if (mailbox_is_readonly(box)) {
		/* silently ignore */
		return 0;
	}

	search_args = mail_search_build_init();
	search_args->args = p_new(search_args->pool, struct mail_search_arg, 1);
	search_args->args->type = SEARCH_FLAGS;
	search_args->args->value.flags = MAIL_DELETED;
	search_args->args->next = next_search_arg;

	/* Refresh the flags so we'll expunge all messages marked as \Deleted
	   by any session. */
	t = mailbox_transaction_begin(box, MAILBOX_TRANSACTION_FLAG_REFRESH,
				      "EXPUNGE");
	ctx = mailbox_search_init(t, search_args, NULL, 0, NULL);
	mail_search_args_unref(&search_args);

	while (mailbox_search_next(ctx, &mail)) {
		*expunged_count += 1;
		mail_expunge(mail);
		expunges = TRUE;
	}

	if (mailbox_search_deinit(&ctx) < 0) {
		mailbox_transaction_rollback(&t);
		return -1;
	} else {
		if (mailbox_transaction_commit(&t) < 0)
			return -1;
	}

	return expunges ? 1 : 0;
}
