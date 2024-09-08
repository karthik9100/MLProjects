# import nltk
import numpy as np
from nltk.stem import WordNetLemmatizer
from collections import Counter
from random import sample
from numpy.linalg import norm
from nltk.corpus import stopwords
import networkx as nx
from rouge_score import rouge_scorer
import string

# Reading the input file
with open("input1.txt", 'r') as f:
    file_content = f.read()

# Splitting sentences and removing leading/trailing whitespace
lis = [i.strip() for i in file_content.split('.') if i.strip()]

# Removing punctuation from sentences
lis = [sentence.translate(str.maketrans("", "", string.punctuation)) for sentence in lis if sentence]

list_of_sentences = lis.copy()
english_stopwords = set(stopwords.words("english"))
lemmatizer = WordNetLemmatizer()

# Lemmatizing and removing stopwords
for i in range(len(lis)):
    lis[i] = [lemmatizer.lemmatize(word.lower()) for word in lis[i].split(" ") if word.lower() not in english_stopwords]

# Maintaining a list of unique words
list_of_words = list(set(word for sentence in lis for word in sentence))

# Initializing the TF-IDF matrix
tf_idf = np.zeros((len(list_of_words), len(list_of_sentences)))

# Calculating TF-IDF
for row in range(len(list_of_words)):
    cur_word = list_of_words[row]
    count = sum(1 for sentence in lis if cur_word in sentence)  # Count of documents containing cur_word
    idf = np.log(len(list_of_sentences) / (count + 1))  # Adding 1 to avoid division by zero

    for col in range(len(list_of_sentences)):
        tf = list_of_sentences[col].count(cur_word)
        tf_idf[row][col] = tf * idf

# Cosine similarity function
def CosineSimilarity(A, B):
    denom = (norm(A) * norm(B)) if A.ndim == 1 else (norm(A, axis=0) * norm(B))
    if denom == 0:
        return 0
    return np.dot(A, B) / denom

n_clusters = 3
centroids = []
random_sample = tf_idf[:, 0]

# Random sampling for initial centroids
for i in range(n_clusters):
    sample1 = sample(list(random_sample), len(list_of_words))
    centroids.append(sample1)

centroids = np.array(centroids)
np.seterr(all="ignore")

# K-means clustering
for _ in range(3):
    clusters = [[] for _ in range(n_clusters)]
    for i in range(len(list_of_sentences)):
        cosine = [CosineSimilarity(centroids[j], tf_idf[:, i]) for j in range(n_clusters)]
        clusters[np.argmax(cosine)].append(i)  # maximal similar sentences are clustered together

    for i in range(n_clusters):
        sentence_index = clusters[i]
        cluster_tf_idf = [tf_idf[:, index] for index in sentence_index]
        if len(cluster_tf_idf) > 0:
            centroids[i] = np.mean(cluster_tf_idf, axis=0)

# Building bigrams
def Build_Bigram(dummysentence):
    words_in_dummy = dummysentence.split(" ")
    bigram = []
    if len(words_in_dummy) > 1:
        for i in range(1, len(words_in_dummy)):
            bigram.append(words_in_dummy[i-1] + "||" + words_in_dummy[i])
    return bigram

# Creating sentence graph
def Obtain_SentenceGraph(S1, S2):
    G = nx.DiGraph()
    G.add_node("start", data="start")
    prev_node = G.nodes["start"]["data"]
    for bi in S1:
        G.add_node(bi, data=bi)
        G.add_edge(prev_node, G.nodes[bi]["data"])
        prev_node = G.nodes[bi]["data"]
    prev_node = G.nodes["start"]["data"]
    for bi in S2:
        if bi in G:
            G.add_edge(prev_node, G.nodes[bi]["data"])
        else:
            G.add_node(bi, data=bi)
            G.add_edge(prev_node, G.nodes[bi]["data"])
        prev_node = G.nodes[bi]["data"]
    G.add_node("end", data="end")
    G.add_edge(prev_node, "end")
    return G

# Generating the summary
final_summary = {}
for clusterindex in range(len(clusters)):
    cosine = [CosineSimilarity(centroids[clusterindex], tf_idf[:, sentenceindex]) for sentenceindex in clusters[clusterindex]]
    if len(cosine) > 0:
        sentence = clusters[clusterindex][np.argmax(cosine)]
        bigrams = [Build_Bigram(list_of_sentences[sentenceindex]) for sentenceindex in clusters[clusterindex]]

        for bigramindex in range(len(bigrams)):
            if bigramindex != np.argmax(cosine):
                count = sum(1 for bi in bigrams[bigramindex] if bi in bigrams[np.argmax(cosine)])
                if count >= 3:
                    G = Obtain_SentenceGraph(bigrams[np.argmax(cosine)], bigrams[bigramindex])
                    summary = nx.shortest_path(G, "start", "end")
                    summary.pop(0)
                    summary.pop(-1)
                    final_summary[sentence] = summary

# Sorting the final summary based on sentence order
myKeys = sorted(final_summary.keys())
sorted_dict = {i: final_summary[i] for i in myKeys}

modified_final_summary = ""
for data in sorted_dict.keys():
    for i in range(len(sorted_dict[data])):
        if i + 1 == len(sorted_dict[data]):
            modified_final_summary += sorted_dict[data][i].split("||")[0] + " "
            modified_final_summary += sorted_dict[data][i].split("||")[1] + "."
        else:
            modified_final_summary += sorted_dict[data][i].split("||")[0] + " "

# Writing the summary to a file
with open("Summary_SentenceGraph.txt", "w") as outputfile:
    outputfile.write(modified_final_summary)

print(modified_final_summary)

# Calculating ROUGE score
reference_summary = """The text discusses COVID-19, an infectious disease caused by the SARS-CoV-2 virus. Most people infected with the virus experience mild to moderate respiratory illness and recover without special treatment. However, some may become seriously ill and require medical attention, especially older adults and those with underlying health conditions like cardiovascular disease or diabetes.

To prevent and slow the spread of the virus, it's essential to stay informed, maintain physical distance (at least 1 meter), wear a properly fitted mask, and practice good hand hygiene. Vaccination is also crucial when eligible. The virus spreads through small liquid particles from an infected person's mouth or nose, particularly when they cough, sneeze, speak, or breathe. Practicing respiratory etiquette, such as coughing into a flexed elbow and self-isolating when unwell, is important for reducing transmission."""
generated_summary = modified_final_summary  # from your code

scorer = rouge_scorer.RougeScorer(['rouge1', 'rouge2', 'rougeL'], use_stemmer=True)
scores = scorer.score(reference_summary, generated_summary)

print("ROUGE-1: ", scores['rouge1'])
print("ROUGE-2: ", scores['rouge2'])
print("ROUGE-L: ", scores['rougeL'])
