import random

def calc_fitness(ch):
    n = len(ch)
    score = 0

    for i in range(n):
        for j in range(i+1, n):
            if ch[i] != ch[j] and abs(ch[i]-ch[j]) != abs(i-j):
                score += 1
    return score

def crossover(p1, p2, split):
    child = p1[:split+1] + p2[split+1:]
    for i in range(len(child)):
        if child.count(child[i]) > 1:
            for v in range(1, len(child)+1):
                if v not in child:
                    child[i] = v
                    break
    return child


def mutation(ch):
    idx = random.randint(0,7)
    new_val = random.randint(1,8)
    ch[idx] = new_val
    return ch


def tournament(pop, fitness):
    a = random.randint(0, len(pop)-1)
    b = random.randint(0, len(pop)-1)

    if fitness[a] > fitness[b]:
        return pop[a]
    else:
        return pop[b]


if __name__ == "__main__":

    P1 = [2,4,1,8,3,6,7,5]
    P2 = [1,5,2,6,3,7,4,8]
    P3 = [4,2,7,1,8,5,3,6]
    P4 = [8,6,4,2,7,5,3,1]

    print("Fitness values")
    print("P1:", calc_fitness(P1))
    print("P2:", calc_fitness(P2))
    print("P3:", calc_fitness(P3))
    print("P4:", calc_fitness(P4))

    print("\nCrossover")
    offA = crossover(P1, P2, 3)
    offB = crossover(P2, P1, 3)

    offC = crossover(P3, P4, 5)
    offD = crossover(P4, P3, 5)

    print("Offspring A:", offA)
    print("Offspring B:", offB)
    print("Offspring C:", offC)
    print("Offspring D:", offD)

    print("\nMutation")
    print("Mutated A:", mutation(offA[:]))
    print("Mutated C:", mutation(offC[:]))


    print("\nRunning GA...")

    pop = [P1[:], P2[:], P3[:], P4[:]]

    for ii in range(16):
        c = list(range(1,9))
        random.shuffle(c)
        pop.append(c)

    best_fit = 0
    best = None
    mut_rate = 0.15

    for gen in range(500):

        fitness = [calc_fitness(c) for c in pop]

        for i in range(len(pop)):
            if fitness[i] > best_fit:
                best_fit = fitness[i]
                best = pop[i][:]

        if best_fit == 28:
            print("Perfect solution found at generation", gen)
            break

        new_pop = []
        best_idx = fitness.index(max(fitness))
        new_pop.append(pop[best_idx][:])

        while len(new_pop) < len(pop):

            p1 = tournament(pop, fitness)
            p2 = tournament(pop, fitness)

            split = random.randint(0,6)

            child1 = crossover(p1, p2, split)
            child2 = crossover(p2, p1, split)

            if random.random() < mut_rate:
                child1 = mutation(child1)

            if random.random() < mut_rate:
                child2 = mutation(child2)

            new_pop.append(child1)
            if len(new_pop) < len(pop):
                new_pop.append(child2)

        pop = new_pop


    print("\nBest fitness:", best_fit, "/ 28")
    print("Best chromosome:", best)