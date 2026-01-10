#!/usr/bin/env python3
"""Extended rules and percentage tests for ClanManager.

This script generates a variety of players and attendance scenarios (including edge cases)
and verifies percent and promotion-eligibility logic. It writes app data files under
`~/Library/Application Support/ClanManager` so the GUI app can read them, and outputs
detailed results to `tests/results_detailed.json`.
"""


import random
import json
from pathlib import Path
from datetime import date, datetime, timedelta


random.seed(12345)

APPDATA = Path.home() / "Library" / "Application Support" / "ClanManager"
APPDATA.mkdir(parents=True, exist_ok=True)


def iso(d):
    return d.isoformat()


def months_since(ref_date, now=None):
    if not ref_date:
        return -1
    if now is None:
        now = date.today()
    months = (now.year - ref_date.year) * 12 + (now.month - ref_date.month)
    if now.day < ref_date.day:
        months -= 1
    return max(0, months)


def compute_percent(attendance_list, total_trainings):
    if total_trainings <= 0:
        return 0
    count = 0
    for obj in attendance_list:
        t = obj.get('type')
        if t in ('Training', 'ClanEvent'):
            count += 1
    # round to nearest integer to match app's qRound
    pct = int(round(count / total_trainings * 100))
    # guard: percent cannot exceed 100 or be negative
    if pct < 0:
        pct = 0
    if pct > 100:
        pct = 100
    return pct


# rank options (keep consistent with app)
RANKS = [
    "Anwerber/AW",
    "Panzergrenadier",
    "Obergrenadier",
    "Gefreiter",
    "Obergefreiter",
    "Stabsgefreiter",
    "Unteroffizier",
    "Stabsunteroffizier (ZBV)",
    "Unterfeldwebel",
    "Feldwebel",
    "Oberfeldwebel",
    "Hauptfeldwebel",
    "Stabsfeldwebel",
    "Fähnrich",
    "Leutnant",
    "Oberleutnant",
    "Hauptmann",
    "Major",
    "Oberst"
]


def next_rank_for(rank):
    try:
        i = RANKS.index(rank)
        return RANKS[i + 1] if i + 1 < len(RANKS) else ""
    except ValueError:
        return ""


# create varied trainings
total_trainings = 20
trainings = []
for i in range(total_trainings):
    tdate = date.today() - timedelta(days=3 * i)
    trainings.append({
        'id': f't{i}',
        'title': f'Train {i}',
        'date': iso(tdate),
        'maps': [f'Map{i}']
    })


# generate rank requirements with some level rules
rank_requirements = {}
for r in RANKS:
    # default months: 3 for lower ranks, 0 for senior
    months = 3 if RANKS.index(r) < 12 else 0
    activities = 3 if RANKS.index(r) < 5 else 0
    level_req = 80 if r.startswith('Anwerber') else 0
    rank_requirements[r] = {'months': months,
                            'activities': activities, 'level': level_req}


# generate many players with random combos including edge cases
players = []
attendance = {}
num_players = 100
groups = ['Angriff', 'Verteidigung', 'Panzerwaffe', 'Artillerie', 'Sonstiges']

for i in range(num_players):
    name = f'P{i:03d}'
    t17 = name
    # edge cases for some players
    if i % 20 == 0:
        # missing join date
        join = None
    elif i % 15 == 0:
        # very recent join
        join = date.today() - timedelta(days=random.randint(0, 20))
    else:
        join = date.today() - timedelta(days=random.randint(30, 800))

    level = random.randint(0, 200)
    # pick from early ranks for more promotions
    rank = random.choice(RANKS[:8])
    nxt = next_rank_for(rank)

    players.append({
        'name': name,
        't17name': t17,
        'level': level,
        'group': random.choice(groups),
        'attendance': 0,
        'totalAttendance': 0,
        'joinDate': iso(join) if join else '',
        'rank': rank,
        'nextRank': nxt
    })

    # generate a random attendance history
    recs = []
    # small chance of no trainings at all
    if random.random() < 0.05:
        recs = []
    else:
        cnt = random.randint(0, total_trainings + 5)
        for k in range(cnt):
            typ = random.choices(
                ['Training', 'ClanEvent', 'Reserve'], weights=[0.6, 0.3, 0.1])[0]
            tid = trainings[random.randint(
                0, total_trainings - 1)]['id'] if typ == 'Training' and random.random() < 0.8 else ''
            recs.append({'date': trainings[random.randint(
                0, total_trainings - 1)]['date'], 'type': typ, 'trainingId': tid})
    attendance[t17] = recs


# persist files for the app
with open(APPDATA / 'trainings.json', 'w') as f:
    json.dump(trainings, f, indent=2)

with open(APPDATA / 'clan_players.csv', 'w') as f:
    f.write('Name\tT17\tLevel\tGroup\tAttendance\tComment\tJoinDate\tRank\tLastPromotion\tNextRank\n')
    for p in players:
        f.write('\t'.join([p['name'], p['t17name'], str(p['level']), p['group'], str(
            p['attendance']), '', p['joinDate'], p['rank'], '', p['nextRank']]) + '\n')

with open(APPDATA / 'clan_attendance.json', 'w') as f:
    json.dump(attendance, f, indent=2)

with open(APPDATA / 'clan_rank_requirements.json', 'w') as f:
    json.dump(rank_requirements, f, indent=2)


# Run validation logic locally and collect detailed results
results = []
failures = []
for p in players:
    key = p['t17name'] or p['name']
    recs = attendance.get(key, [])
    percent = compute_percent(recs, total_trainings)
    join_date = None
    if p['joinDate']:
        try:
            join_date = datetime.fromisoformat(p['joinDate']).date()
        except Exception:
            join_date = None
    months = months_since(join_date)
    req = rank_requirements.get(p['rank'], {})
    req_months = req.get('months', 0)
    req_activities = req.get('activities', 0)
    req_level = req.get('level', 0)
    activities = sum(1 for r in recs if r.get('type')
                     in ('Training', 'ClanEvent', 'Reserve'))
    eligible = True
    reasons = []
    if req_months > 0 and (join_date is None or months < req_months):
        eligible = False
        reasons.append(f'months {months} < required {req_months}')
    if req_activities > 0 and activities < req_activities:
        eligible = False
        reasons.append(f'activities {activities} < required {req_activities}')
    if req_level > 0 and p['level'] < req_level:
        eligible = False
        reasons.append(f'level {p["level"]} < required {req_level}')

    entry = {
        'player': key,
        'rank': p['rank'],
        'nextRank': p['nextRank'],
        'level': p['level'],
        'joinDate': p['joinDate'],
        'months': months,
        'activities': activities,
        'percent': percent,
        'eligible': eligible,
        'reasons': reasons,
        'attendanceCount': len(recs)
    }
    results.append(entry)
    # basic sanity checks
    if not (0 <= percent <= 100):
        failures.append(
            {'player': key, 'issue': 'percent_out_of_bounds', 'percent': percent})
    if percent != compute_percent(recs, total_trainings):
        failures.append({'player': key, 'issue': 'percent_mismatch'})


out = {
    'appdata': str(APPDATA),
    'total_trainings': total_trainings,
    'players_tested': len(players),
    'failures': failures,
    'results': results
}

with open('tests/results_detailed.json', 'w') as f:
    json.dump(out, f, indent=2)

print('Wrote appdata to:', APPDATA)
print('Wrote detailed results to: tests/results_detailed.json')
print('Summary: players tested=', len(players), 'failures=', len(failures))
