class process
{
	public:
		process():m_pid(-1){}
	public:
		pid_t m_pid;
		int m_pipefd[2];
};
