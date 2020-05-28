import pickle
import sys

keepers =[
    "hyperu_arg3",
    "gsl_sf_hyperg_U",
    "factorial_arg1",
    "fac_arg1",
    "altzeta_arg1",
    "gsl_sf_eta",
    "eta",
    "expint_arg2",
    "expn_arg2",
    "gsl_sf_expint_En",
    "bessely_arg2",
    "yn_arg2",
    "yv_arg2",
    "yve_arg2",
    "bessely",
    "gsl_sf_bessel_Yn",
    "besselj_arg2",
    "jn_arg2",
    "jv_arg2",
    "jve_arg2",
    "gsl_sf_bessel_Jn",
    "angerj"
]

def prune(file_name):

    new_sig={}

    with open(file_name, "rb") as f:
        all_sig = pickle.load(f)

    print(f"Pruning {len(all_sig)} extracted signatures")

    for key, value in all_sig.items():
        if any(x in key for x in keepers):
            new_sig[key] = value

    print(f"{len(new_sig)} signatures remain")

    with open(file_name, "wb") as f:
        pickle.dump(new_sig, f)

if __name__ == "__main__":
    prune(sys.argv[1])