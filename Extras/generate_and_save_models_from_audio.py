# -*- coding: utf-8 -*-

import argparse
import os
import sys

sys.path.append("../AutoencoderJuce/external/frugally-deep/keras_export/")

import convert_model

def make_autoencoder(filename, epochs=1, hop=None, win=None):
    # Utils
    from glob import glob
    import tqdm
    from pathlib import Path

    import datetime

    # Numeros
    import numpy as np
    from scipy.interpolate import interp1d

    # Redes neuronales
    from keras.utils.vis_utils import model_to_dot, plot_model
    from keras.models import Sequential, Model
    from keras.layers import Input, Dense, Conv2D, MaxPooling2D, UpSampling2D, Flatten, Reshape, BatchNormalization
    from keras.initializers import RandomNormal
    from keras.optimizers import RMSprop, Adam
    from keras import backend as K
    from keras import regularizers
    from keras.datasets import mnist
    import keras as keras
    import tensorflow as tf
    import tensorflowjs as tfjs

    # Machine learning
    from sklearn.manifold import TSNE
    from sklearn.model_selection import train_test_split
    from sklearn.decomposition import PCA

    # Audio
    import librosa
    import librosa.display

    settings = {}
    sClip = -60
    settings["sClip"] = str(float(sClip))
    hop_length_ms = 10
    duration = 60 * 2
    settings["sr"] = 22050
    hop_length = hop or int(hop_length_ms / 1000 * settings["sr"])
    settings["win_length"] = win or hop_length * 4

    X = []
    y = []
    phases = []

    for i,filename in enumerate(glob(filename)):
        x,sr = librosa.load(filename, sr=settings["sr"], mono=True,duration=duration)
        x = np.trim_zeros(x)
        F = librosa.stft(x,n_fft=settings["win_length"], hop_length=hop_length).T
        phases.append(np.angle(F))
        S = 10*np.log10(np.abs(F)**2)
        S = S.clip(sClip, None)-sClip
        y.append(np.ones(S.shape[0])*i)
        X.append(S)

    phases = np.vstack(phases)
    X = np.vstack(X)
    settings["xMax"] = str(X.max())
    X = X / X.max()
    y = np.hstack(y)

    n_features = X.shape[1]

    # Separamos en train y val

    X_train, X_val, y_train, y_val = train_test_split(X, y,  test_size=0.05, shuffle=True)

    # Armamos un autoencoder denso de 8 capas

    K.clear_session()

    input_mag = Input(shape=(n_features,))

    kr_init = keras.initializers.glorot_normal()

    layers = [512,256,128,64,32,16,8]
    latent_dim = layers[-1]

    settings["latent_dim"] = latent_dim

    settings["zRange"] = [{ "min" : "-5.0", "max" : "5.0" } for i in range(latent_dim)]

    x = input_mag
    for l in layers:
        x = BatchNormalization()(x)
        x = Dense(l, activation='relu', kernel_initializer=kr_init)(x)
    encoded = x

    encoder = Model(input_mag, encoded)
    latent_inputs = Input(shape=(latent_dim,), name='z')
    x = latent_inputs
    for l in layers[::-1][1:]:
        x = BatchNormalization()(x)
        x = Dense(l, activation='relu', kernel_initializer=kr_init)(x)
    decoded = Dense(n_features, activation='relu', kernel_initializer=kr_init)(x)

    decoder = Model(latent_inputs, decoded, name='decoder')

    outputs = decoder(encoder(input_mag))
    autoencoder = Model(input_mag, outputs, name='autoencoder')

    opt = RMSprop(0.005)
    opt = Adam(0.002, clipnorm=0.5)

    autoencoder.compile(optimizer=opt, loss='mse', metrics=['mse'])

    logdir = Path("logs", datetime.datetime.now().strftime("%Y%m%d-%H%M%S"))
    tensorboard_callback = keras.callbacks.TensorBoard(logdir, histogram_freq=1)

    silence = np.zeros([int(X_train.shape[0]*0.1),X_train.shape[1]])

    autoencoder.fit(np.vstack([silence, X_train]), np.vstack([silence, X_train]),
                    epochs=epochs,
                    batch_size=128,
                    shuffle=True, 
                    validation_data= (X_val,X_val),
                    callbacks=[tensorboard_callback])
    
    # save models
    # encoder.save('encoder')
    decoder.save('decoder')
    # autoencoder.save('autoencoder')

    print("\n\nModel exported to file 'decoder'\n\n")

    return settings

def main():
    """Do stuff"""

    parser = argparse.ArgumentParser(description='Create a simple autoencoder, train it with some input wave file and output the saved models')
    parser.add_argument('-a', '--audio', help="WAVE file to build upon the autoencoder", required=True)
    parser.add_argument('-e', '--epochs', help="Number of epochs for fitting", required=False)
    parser.add_argument('-j', '--hop', help="Size of hop length in samples", required=False)
    parser.add_argument('-w', '--win', help="Window length in samples", required=False)
    parser.add_argument('-m', '--model_only', help="Don't convert output model using Frugally Deep's script", required=False, action='store_true')
    args = parser.parse_args()

    epochs = int(args.epochs) if args.epochs else None
    hop_length = int(args.hop) if args.hop else None
    win_length = int(args.win) if args.win else None

    json_output = { "parameters" : make_autoencoder(args.audio, epochs, hop_length, win_length) }

    if not args.model_only:
        print("\n\nDecoder model will now be converted using Frugally Deep's script'\n\n")
        # convert_model.convert("./decoder", "decoder.json", True)
        model = convert_model.load_model("./decoder")

        json_output["model"] = convert_model.model_to_fdeep_json(model, True)

        import json

        with open('data.json', 'w', encoding='utf-8') as f:
            json.dump(json_output, f, ensure_ascii=False, indent=2, allow_nan=False, sort_keys=True)

        os.remove('./decoder')
        print("\n\nDONE. You can now load 'decoder.json' into the application\n\n")
    else:
        print("\n\nDon't forget to convert your models using Frugally Deep's scripts before using with the application\n\n")


if __name__ == "__main__":
    main()

