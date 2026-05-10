#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spawn.h"

/* Opens path thorugh the system editor (defaults to vi). */
bool fork_to_editor(const char *path)
{
	const char *editor = getenv("VISUAL");
	if (!editor)
		editor = getenv("EDITOR");
	if (!editor)
		editor = "vi";

	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		return false;
	}

	if (pid == 0) {
		/*type cast to silence compiler warning about const modifier */
		char *args[] = {(char *)editor, (char *)path, NULL};
		execvp(editor, args);

		perror("execvp");
		exit(1);
	}

	int status;
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return false;
	}

	if (WIFEXITED(status)) {
		fprintf(stderr,
			"Editor exited normally with code %d\n",
			WEXITSTATUS(status));
		return true;
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr,
			"Editor was killed by signal %d (%s)\n",
			WTERMSIG(status),
			strsignal(WTERMSIG(status)));
		return true;
	}

	fprintf(stderr, "Editor ended in an unexpected way\n");
	return false;
}

/* Opens path thorugh the system pager (defaults to less). */
bool fork_to_pager(const char *path)
{
	const char *pager = getenv("PAGER");
	if (!pager)
		pager = "less";
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return false;
	}
	if (pid == 0) {
		char *args[] = {(char *)pager, (char *)path, NULL};
		execvp(pager, args);
		perror("execvp");
		exit(1);
	}
	int status;
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return false;
	}
	if (WIFEXITED(status)) {
		fprintf(stderr,
			"Pager exited normally with code %d\n",
			WEXITSTATUS(status));
		return true;
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr,
			"Pager was killed by signal %d (%s)\n",
			WTERMSIG(status),
			strsignal(WTERMSIG(status)));
		return true;
	}
	fprintf(stderr, "Pager ended in an unexpected way\n");
	return false;
}

/* Pipes output of writer function to the system pager (defaults to less). */
bool pipe_to_pager(void (*writer)(void *ctx), void *ctx)
{
	int saved_stdout = dup(STDOUT_FILENO);
	if (saved_stdout == -1) {
		perror("dup");
		return false;
	}

	int pipefd[2];
	if (pipe(pipefd) == -1) {
		perror("pipe");
		close(saved_stdout);
		return false;
	}

	const char *pager = getenv("PAGER");
	if (!pager)
		pager = "less";

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		close(pipefd[0]);
		close(pipefd[1]);
		close(saved_stdout);
		return false;
	}

	if (pid == 0) {
		close(pipefd[1]);
		if (dup2(pipefd[0], STDIN_FILENO) == -1) {
			perror("dup2");
			exit(1);
		}
		close(pipefd[0]);
		char *args[] = {(char *)pager, NULL};
		close(saved_stdout);
		execvp(pager, args);

		perror("execvp");
		exit(1);
	}

	close(pipefd[0]);
	if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
		perror("dup2");
		close(pipefd[1]);
		close(saved_stdout);
		return false;
	}
	close(pipefd[1]);

	writer(ctx);

	fflush(stdout);

	/* Skip fclose of stdout to avoid the C library represantion to remain marked
	 * as close. The OS closes the old fd, keeping stdout open */
	if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
		perror("dup2 restore");
		close(saved_stdout);
		return false;
	}
	close(saved_stdout);

	int status;
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return false;
	}
	if (WIFEXITED(status)) {
		fprintf(stderr,
			"Pager exited normally with code %d\n",
			WEXITSTATUS(status));
		return true;
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr,
			"Pager was killed by signal %d (%s)\n",
			WTERMSIG(status),
			strsignal(WTERMSIG(status)));
		return true;
	}
	fprintf(stderr, "Pager ended in an unexpected way\n");
	return false;
}
