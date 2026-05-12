#!/bin/bash
#SBATCH --job-name=blast_container
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=16G
#SBATCH --time=00:15:00
#SBATCH --partition=dcgp_usr_prod
#SBATCH --account=your_account
#SBATCH --output=%x.%j.out
#SBATCH --error=%x.%j.err

# Redirect cache to scratch
export SINGULARITY_CACHEDIR=$SCRATCH/.singularity_cache

# Use the pre-pulled SIF (do not pull inside a job — network may be restricted)
SIF=$SCRATCH/.singularity_cache/blast_latest.sif

# Use multiple threads via BLAST's -num_threads option
singularity exec $SIF blastn \
    -query $SCRATCH/test_query.fasta \
    -db    $SCRATCH/test_blastdb \
    -out   $SCRATCH/blast_batch_results.txt \
    -num_threads ${SLURM_CPUS_PER_TASK} \
    -outfmt 6

echo "BLAST job completed on node: $(hostname)"
echo "Threads used: ${SLURM_CPUS_PER_TASK}"
