# Use an official Ubuntu base image
FROM ubuntu:20.04

# Set noninteractive installation to avoid getting stuck during the build
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages for a Slurm worker node and MPI
RUN apt-get update && apt-get install -y --no-install-recommends \
    munge \
    libmunge2 \
    libmunge-dev \
    slurm-wlm \
    slurm-client \
    ssh \
    vim \
    iputils-ping \
    libfftw3-3 libfftw3-mpi-dev libfftw3-dev clang\
    openmpi-bin openmpi-common \
    && rm -rf /var/lib/apt/lists/*

# Set up Munge (authentication service for creating and validating credentials)
RUN create-munge-key
COPY munge.key /etc/munge/munge.key
RUN chown munge.munge /etc/munge/munge.key
RUN chmod 400 /etc/munge/munge.key
# Configure SSH for MPI communication between nodes
RUN mkdir /var/run/sshd
RUN echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config
# SSH login fix. Otherwise, user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
RUN groupadd -r pinocchio -g 10000 && useradd -r -g pinocchio -u 10000 -s /bin/bash user01 && mkdir -p /home/user01/.ssh && chown -R user01.pinocchio /home/user01/

# Expose the necessary ports
EXPOSE 22 6818

# Set the entrypoint and command to start services
COPY entrypoint.sh /usr/local/bin/worker-entrypoint.sh
RUN chmod +x /usr/local/bin/worker-entrypoint.sh
ENTRYPOINT ["/usr/local/bin/worker-entrypoint.sh"]
CMD ["/usr/sbin/sshd", "-D"]
