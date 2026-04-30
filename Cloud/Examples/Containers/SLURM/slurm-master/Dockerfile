# Use an official Ubuntu base image
FROM ubuntu:20.04

# Set noninteractive installation to avoid getting stuck during the build
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    munge \
    libmunge2 \
    libmunge-dev \
    slurm-wlm \
    slurm-wlm-basic-plugins \
    slurm-wlm-torque \
    gcc \
    clang \
    ssh \
    vim git \
    iputils-ping \
    openmpi-bin openmpi-common \
    libfftw3-3 libfftw3-mpi-dev libfftw3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set up Munge (authentication service for creating and validating credentials)
RUN create-munge-key
COPY munge.key /etc/munge/munge.key
RUN chown munge.munge /etc/munge/munge.key && chmod 400 /etc/munge/munge.key

# Configure SSH for MPI communication between nodes
RUN mkdir /var/run/sshd
RUN echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config
# SSH login fix. Otherwise, user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
RUN mkdir /root/.ssh
COPY id_rsa.pub /root/.ssh/authorized_keys
RUN chown root.root /root/.ssh/authorized_keys && chmod 600 /root/.ssh/authorized_keys
# Expose ports for SSH and Slurm
EXPOSE 22 6817

# Copy the Slurm configuration files
COPY slurm.conf /etc/slurm-llnl/slurm.conf
RUN groupadd -r pinocchio -g 10000 && useradd -r -g pinocchio -u 10000 -s /bin/bash user01 && mkdir -p /home/user01/.ssh && chown -R user01.pinocchio /home/user01/
COPY id_rsa.pub /home/user01/.ssh/authorized_keys
RUN chown user01.pinocchio /home/user01/.ssh/authorized_keys && chmod 600 /home/user01/.ssh/authorized_keys
# Set up entrypoint and default command
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["/usr/sbin/sshd", "-D"]

