"""
Convert a PyTorch model to an ONNX graph needed for Tensil compilation
Save :
    - onnx model file (.onnx)
"""

import torch
import torchvision
import argparse
import timm
import numpy as np
from tqdm import tqdm

# Set device to CPU
device = torch.device("cpu")

def load_resnet50_imagenet(args):
    args.input_resolution = 224
    args.input_model = "resnet50_imagenet.pt"
    model = timm.create_model("resnet50.a1_in1k", pretrained=True)
    model.eval()
    return model
    
def load_resnet18_imagenet(args):
    args.input_resolution = 224
    args.input_model = "resnet18_imagenet.pt"
    model = timm.create_model("resnet18.a1_in1k", pretrained=True)
    model.eval()
    return model

def load_resnet50_bee306(args):
    args.input_resolution = 224
    nb_classes = 306
    model = timm.create_model("resnet50.a1_in1k", pretrained=True, num_classes=nb_classes)
    model.load_state_dict(torch.load(args.input_model, map_location=torch.device('cpu'))['state_dict'])
    model.eval()
    return model

def load_resnet50_tipu12(args):
    args.input_resolution = 224
    nb_classes = 12
    model = timm.create_model("resnet50.a1_in1k", pretrained=True, num_classes=nb_classes)
    state_dict = {}
    state_dict = torch.load(args.input_model, map_location=torch.device('cpu'))
    # Remove "0." from the keys
    new_state_dict = {}
    for k, v in state_dict.items():
        name = k[2:] # remove `module.`
        new_state_dict[name] = v
    model.load_state_dict(new_state_dict)
    model.eval()
    return model

def pytorch_to_onnx(args, model):
    model_name = args.input_model.split("/")[-1].split(".")[0]
    print("Number of parameters: " + str(sum(p.numel() for p in model.parameters())))

    dummy_input = torch.randn(1, 3, args.input_resolution, args.input_resolution)
    torch.onnx.export(model, dummy_input, args.output_dir + model_name + ".onnx",
                      verbose=True, input_names=[args.input_names], output_names=[args.output_names], opset_version=10) # Tensil only supports opset 9 and 10
    print("Model exported to " + args.output_dir + ".onnx")


if __name__ == "__main__":
    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-model", "-m", type=str, default="resnet50_bee306_best.pth.tar", help="Path to the input PyTorch model")
    parser.add_argument("--output-dir", "-d", type=str, default="", help="Directory to save the ONNX model")
    parser.add_argument("--input-resolution", "-r", type=int, default=224, help="Input resolution of the model. Default: 224")
    parser.add_argument("--input-names", "-i", type=str, default="x:0", help="Name of the input tensor")
    parser.add_argument("--output-names", "-o", type=str, default="Identity:0", help="Name of the output tensor")
    args = parser.parse_args()

    # model = load_resnet50_imagenet(args)
    # model = load_resnet50_bee306(args)
    model = load_resnet50_tipu12(args)
    # model = load_resnet18_imagenet(args)

    pytorch_to_onnx(args, model)
