#!/usr/bin/env python3
import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import random

def parse_file(path):
    events = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                left, ts = line.split('---')
            except ValueError:
                continue  # skip malformed lines
            tag, name = left.strip().split(' ', 1)
            events.append((tag, name, float(ts)))
    return events

def compute_durations(events):
    starts = {}
    durations = defaultdict(list)
    for tag, name, t in events:
        if tag == 'PROFILESTART':
            starts[name] = t
        elif tag == 'PROFILEEND':
            if name in starts:
                start = starts.pop(name)
                durations[name].append((start, t - start))
    return durations

def plot_durations(durations, normalize=True):
    # find earliest start
    all_starts = [s for segs in durations.values() for s,_ in segs]
    offset = min(all_starts) if normalize and all_starts else 0

    names = list(durations.keys())
    colors = {name: (random.random(), random.random(), random.random()) for name in names}
    fig, ax = plt.subplots(figsize=(10, 0.5*len(names) + 1))
    bar_h = 0.8

    for i, name in enumerate(names):
        segs = [ (s - offset, d) for s,d in durations[name] ]
        ax.broken_barh(segs, (i-bar_h/2, bar_h),
                       facecolors=colors[name],
                       edgecolors='black',
                       label=name)

    ax.set_yticks(range(len(names)))
    ax.set_yticklabels(names)
    ax.set_xlabel('Time (ms)')
    ax.set_title('Profile Durations (normalized to 0)')
    plt.tight_layout()
    plt.show()

def main():
    p = argparse.ArgumentParser(description="Plot PROFILESTART/END durations")
    p.add_argument('file', help="path to log file")
    args = p.parse_args()

    ev = parse_file(args.file)
    durs = compute_durations(ev)
    plot_durations(durs)

if __name__ == '__main__':
    main()
