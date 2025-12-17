import os
import tkinter as tk
from tkinter import filedialog, messagebox
import pandas as pd
import vobject
from email.header import decode_header


def decode_quoted_printable(text):
    if text.startswith("=?") and "?=" in text:
        decoded, encoding = decode_header(text)[0]
        if isinstance(decoded, bytes):
            return decoded.decode(encoding or 'utf-8', errors='ignore')
        return decoded
    return text


def parse_vcf(file_path):
    contacts = []
    errors = 0
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
            vcard_data = file.read()
            for vcard in vobject.readComponents(vcard_data):
                try:
                    contact = {}
                    if hasattr(vcard, 'fn'):
                        contact['Nome'] = decode_quoted_printable(vcard.fn.value)
                    else:
                        contact['Nome'] = 'Desconhecido'
                    
                    if hasattr(vcard, 'tel'):
                        
                        contact['Telefone'] = vcard.tel.value.replace('+', '').replace(' ', '')
                    else:
                        contact['Telefone'] = ''
                    
                    contacts.append(contact)
                except Exception as e:
                    errors += 1
                    print(f"Erro ao processar vCard: {e}")
    except Exception as e:
        print(f"Erro ao ler o arquivo VCF: {e}")
    return contacts, errors


def save_to_file(contacts, file_path, file_format):
    df = pd.DataFrame(contacts)
    if file_format == 'CSV':
        df.to_csv(file_path, index=False)
    elif file_format == 'TSV':
        df.to_csv(file_path, sep='\t', index=False)
    elif file_format == 'XLS':
        df.to_excel(file_path, index=False, engine='openpyxl')
    elif file_format == 'XLSX':
        df.to_excel(file_path, index=False, engine='openpyxl')


def convert_vcf():
    vcf_path = entry_vcf.get()
    output_format = format_var.get()
    output_dir = entry_output.get()

    if not vcf_path or not output_format or not output_dir:
        messagebox.showerror("Erro", "Por favor, preencha todos os campos.")
        return

    contacts, errors = parse_vcf(vcf_path)

    if not contacts:
        messagebox.showerror("Erro", "Nenhum contato encontrado no arquivo VCF.")
        return

    base_name = 'contatos'
    if len(contacts) <= 1000:
        file_path = os.path.join(output_dir, f'{base_name}.{output_format.lower()}')
        save_to_file(contacts, file_path, output_format)
    else:
        for i in range(0, len(contacts), 1000):
            chunk = contacts[i:i+1000]
            file_path = os.path.join(output_dir, f'{base_name}_{i//1000 + 1}.{output_format.lower()}')
            save_to_file(chunk, file_path, output_format)

   
    messagebox.showinfo("Sucesso", f"Conversão realizada com sucesso!\nContatos salvos: {len(contacts)}\nErros encontrados: {errors}")


def select_vcf():
    file_path = filedialog.askopenfilename(filetypes=[("VCF files", "*.vcf")])
    entry_vcf.delete(0, tk.END)
    entry_vcf.insert(0, file_path)

def select_output():
    dir_path = filedialog.askdirectory()
    entry_output.delete(0, tk.END)
    entry_output.insert(0, dir_path)

# Interface Gráfica tk 
root = tk.Tk()
root.title("Conversor VCF para Planilha")


tk.Label(root, text="Arquivo VCF:").grid(row=0, column=0, padx=10, pady=10)
entry_vcf = tk.Entry(root, width=50)
entry_vcf.grid(row=0, column=1, padx=10, pady=10)
tk.Button(root, text="Selecionar", command=select_vcf).grid(row=0, column=2, padx=10, pady=10)


tk.Label(root, text="Destino:").grid(row=1, column=0, padx=10, pady=10)
entry_output = tk.Entry(root, width=50)
entry_output.insert(0, os.path.expanduser("~/Desktop"))
entry_output.grid(row=1, column=1, padx=10, pady=10)
tk.Button(root, text="Selecionar", command=select_output).grid(row=1, column=2, padx=10, pady=10)


tk.Label(root, text="Formato de Saída:").grid(row=2, column=0, padx=10, pady=10)
format_var = tk.StringVar(value="CSV")
formats = ["CSV", "TSV", "XLS", "XLSX"]
for i, fmt in enumerate(formats):
    tk.Radiobutton(root, text=fmt, variable=format_var, value=fmt).grid(row=2, column=i+1, padx=10, pady=10)

#iniciar a conversão
tk.Button(root, text="Converter", command=convert_vcf).grid(row=3, column=1, padx=10, pady=20)


root.mainloop()