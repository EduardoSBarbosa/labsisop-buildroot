import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def main():
    # Configurações de arquivos
    arquivo_csv = 'disktest/scan_info.txt'
    pasta_saida = 'graphs'
    os.makedirs(pasta_saida, exist_ok=True)

    # 1. Carga dos dados
    try:
        df = pd.read_csv(arquivo_csv)
    except FileNotFoundError:
        print(f"Erro: O arquivo '{arquivo_csv}' não foi encontrado.")
        return

    # 2. Pré-processamento e Cálculo de Métricas
    # Converte nanossegundos para milissegundos para facilitar a leitura
    df['Tempo_Medio_ms'] = df['Tempo_Medio_ns'] / 1_000_000
    df['Tempo_Maximo_ms'] = df['Tempo_Maximo_ns'] / 1_000_000
    
    # MÉTRICA CHAVE: Calcula a distância média por deslocamento
    # Isso resolve a distorção de escala entre testes diferentes
    df['Seek_Medio_por_Requisicao'] = df['Total_Seek'] / df['Total_Reqs']

    # 3. Configuração Estética do Seaborn
    sns.set_theme(style="whitegrid")
    plt.rcParams["figure.figsize"] = (12, 7)
    plt.rcParams["figure.dpi"] = 300

    print(f"Iniciando geração dos gráficos comparativos em '{pasta_saida}'...")

    # --- GRÁFICO 1: EFICIÊNCIA DE MOVIMENTO (Seek Médio) ---
    plt.figure()
    ax1 = sns.lineplot(
        data=df, x='Workers', y='Seek_Medio_por_Requisicao', 
        hue='Metodo', style='Metodo', markers=True, linewidth=2.5
    )
    plt.title('Eficiência do Escalonador: Distância Média por Seek\n(SSTF: Sequential vs Random)', fontsize=14, pad=15)
    plt.ylabel('Setores percorridos por requisição (Média)', fontsize=12)
    plt.xlabel('Número de Workers (Processos Concorrentes)', fontsize=12)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, '1_seek_medio_por_req.jpg'))
    plt.close()

    # --- GRÁFICO 2: TEMPO MÉDIO DE RESPOSTA ---
    plt.figure()
    ax2 = sns.lineplot(
        data=df, x='Workers', y='Tempo_Medio_ms', 
        hue='Metodo', style='Metodo', markers=True, linewidth=2.5
    )
    plt.title('Latência Média: Tempo de Resposta vs Workers', fontsize=14, pad=15)
    plt.ylabel('Tempo Médio (ms)', fontsize=12)
    plt.xlabel('Número de Workers', fontsize=12)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, '2_tempo_medio_resposta.jpg'))
    plt.close()

    # --- GRÁFICO 3: PIOR CASO / STARVATION (Escala Logarítmica) ---
    plt.figure()
    ax3 = sns.lineplot(
        data=df, x='Workers', y='Tempo_Maximo_ms', 
        hue='Metodo', style='Metodo', markers=True, linewidth=2.5
    )
    plt.yscale('log') # Escala logarítmica para destacar a explosão do Starvation
    plt.title('Análise de Pior Caso: Tempo Máximo de Resposta\n(Evidência de Starvation no SSTF Random)', fontsize=14, pad=15)
    plt.ylabel('Tempo Máximo (ms) - Escala Logarítmica', fontsize=12)
    plt.xlabel('Número de Workers', fontsize=12)
    plt.tight_layout()
    plt.savefig(os.path.join(pasta_saida, '3_tempo_maximo_starvation.jpg'))
    plt.close()

    print(f"\nSucesso! 3 gráficos gerados na pasta '{pasta_saida}':")
    print("1. 1_seek_medio_por_req.jpg      -> Mostra como o SSTF agrupa as requisições.")
    print("2. 2_tempo_medio_resposta.jpg    -> Mostra a latência média do sistema.")
    print("3. 3_tempo_maximo_starvation.jpg -> Mostra o Starvation (o ponto fraco do SSTF).")

if __name__ == "__main__":
    main()