package app.tuxguitar.player.impl.midiport.nam;

import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.List;

import app.tuxguitar.app.TuxGuitar;
import app.tuxguitar.app.action.impl.view.TGOpenViewAction;
import app.tuxguitar.app.system.icons.TGIconManager;
import app.tuxguitar.app.ui.TGApplication;
import app.tuxguitar.app.util.TGFileChooser;
import app.tuxguitar.app.view.dialog.file.TGFileChooserDialog;
import app.tuxguitar.app.view.dialog.file.TGFileChooserHandler;
import app.tuxguitar.app.view.dialog.message.TGMessageDialog;
import app.tuxguitar.app.view.dialog.message.TGMessageDialogController;
import app.tuxguitar.app.view.util.TGDialogUtil;
import app.tuxguitar.editor.action.TGActionProcessor;
import app.tuxguitar.io.base.TGFileFormat;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUI;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUICallback;
import app.tuxguitar.ui.UIFactory;
import app.tuxguitar.ui.event.UISelectionEvent;
import app.tuxguitar.ui.event.UISelectionListener;
import app.tuxguitar.ui.layout.UITableLayout;
import app.tuxguitar.ui.widget.UIButton;
import app.tuxguitar.ui.widget.UILabel;
import app.tuxguitar.ui.widget.UILegendPanel;
import app.tuxguitar.ui.widget.UIReadOnlyTextField;
import app.tuxguitar.ui.widget.UISlider;
import app.tuxguitar.ui.widget.UIWindow;
import app.tuxguitar.util.TGContext;

public class NamAudioProcessorUI implements TGAudioProcessorUI, UISelectionListener {

	private static final List<TGFileFormat> NAM_FORMATS = Arrays.asList(
		new TGFileFormat("NAM Model", "application/x-nam", new String[] { "nam" }));

	private TGContext context;
	private TGAudioProcessorUICallback callback;
	private NamAudioProcessor processor;

	private UIWindow dialog;
	private UIReadOnlyTextField modelName;
	private UISlider gainSlider;
	private UILabel gainValueLabel;
	private boolean updating;

	public NamAudioProcessorUI(TGContext context, NamAudioProcessor processor, TGAudioProcessorUICallback callback) {
		this.context = context;
		this.processor = processor;
		this.callback = callback;
	}

	public String getLabel() {
		String path = this.processor.getModelPath();
		if (path != null) {
			return "NAM: " + new File(path).getName();
		}
		return "NAM";
	}

	public void open(final UIWindow parent) {
		UIFactory uiFactory = TGApplication.getInstance(this.context).getFactory();
		UITableLayout dialogLayout = new UITableLayout();

		this.dialog = uiFactory.createWindow(parent, false, false);
		this.dialog.setLayout(dialogLayout);
		this.dialog.setText("NAM - Neural Amp Modeler");

		// ------------------------------------------------------------------
		// Model (.nam)
		// ------------------------------------------------------------------
		UITableLayout modelLayout = new UITableLayout();
		UILegendPanel modelGroup = uiFactory.createLegendPanel(this.dialog);
		modelGroup.setLayout(modelLayout);
		modelGroup.setText("Model (.nam)");
		dialogLayout.set(modelGroup, 1, 1, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_FILL, true, true);

		this.modelName = uiFactory.createReadOnlyTextField(modelGroup);
		modelLayout.set(this.modelName, 1, 1, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_CENTER, true, false, 1, 1, 300f, null, null);

		UIButton clearButton = uiFactory.createButton(modelGroup);
		clearButton.setImage(TuxGuitar.getInstance().getIconManager().getImageByName(TGIconManager.LIST_REMOVE));
		clearButton.setToolTipText("Clear");
		clearButton.addSelectionListener(new UISelectionListener() {
			public void onSelect(UISelectionEvent event) {
				NamAudioProcessorUI.this.applyModel(null);
			}
		});
		modelLayout.set(clearButton, 1, 2, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_FILL, false, false);

		UIButton browseButton = uiFactory.createButton(modelGroup);
		browseButton.setImage(TuxGuitar.getInstance().getIconManager().getImageByName(TGIconManager.FILE_OPEN));
		browseButton.setToolTipText("Browse...");
		browseButton.addSelectionListener(new UISelectionListener() {
			public void onSelect(UISelectionEvent event) {
				NamAudioProcessorUI.this.chooseModel(parent);
			}
		});
		modelLayout.set(browseButton, 1, 3, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_FILL, false, false);

		// ------------------------------------------------------------------
		// Output gain
		// ------------------------------------------------------------------
		UITableLayout gainLayout = new UITableLayout();
		UILegendPanel gainGroup = uiFactory.createLegendPanel(this.dialog);
		gainGroup.setLayout(gainLayout);
		gainGroup.setText("Output gain");
		dialogLayout.set(gainGroup, 2, 1, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_FILL, true, true);

		this.gainSlider = uiFactory.createHorizontalSlider(gainGroup);
		this.gainSlider.setMinimum(0);
		this.gainSlider.setMaximum(200);
		this.gainSlider.setIncrement(5);
		this.gainSlider.addSelectionListener(this);
		gainLayout.set(this.gainSlider, 1, 1, UITableLayout.ALIGN_FILL, UITableLayout.ALIGN_CENTER, true, false);

		this.gainValueLabel = uiFactory.createLabel(gainGroup);
		gainLayout.set(this.gainValueLabel, 1, 2, UITableLayout.ALIGN_CENTER, UITableLayout.ALIGN_CENTER, false, false);

		this.updateItems();

		TGDialogUtil.openDialog(this.dialog, TGDialogUtil.OPEN_STYLE_CENTER | TGDialogUtil.OPEN_STYLE_PACK);
	}

	public void chooseModel(UIWindow parent) {
		try {
			TGFileChooser.getInstance(this.context).openChooser(new TGFileChooserHandler() {
				public void updateFileName(String file) {
					try {
						NamAudioProcessorUI.this.applyModel(file);
					} catch (Throwable e) {
						NamAudioProcessorUI.this.showError(e);
					}
				}
			}, NAM_FORMATS, TGFileChooserDialog.STYLE_OPEN);
		} catch (Throwable e) {
			this.showError(e);
		}
	}

	private void showError(Throwable e) {
		e.printStackTrace();
		StringWriter stackTrace = new StringWriter();
		e.printStackTrace(new PrintWriter(stackTrace));
		TGActionProcessor tgActionProcessor = new TGActionProcessor(this.context, TGOpenViewAction.NAME);
		tgActionProcessor.setAttribute(TGOpenViewAction.ATTRIBUTE_CONTROLLER, new TGMessageDialogController());
		tgActionProcessor.setAttribute(TGMessageDialog.ATTRIBUTE_TITLE, "TG-NAM");
		tgActionProcessor.setAttribute(TGMessageDialog.ATTRIBUTE_MESSAGE, e.getClass().getName() + ": " + e.getMessage() + "\n\n" + stackTrace.toString());
		tgActionProcessor.setAttribute(TGMessageDialog.ATTRIBUTE_STYLE, TGMessageDialog.STYLE_ERROR);
		tgActionProcessor.process();
	}

	public void applyModel(String path) {
		this.processor.setModelPath(path);
		this.updateItems();
		this.callback.onChange(false);
	}

	public void updateItems() {
		this.updating = true;
		String path = this.processor.getModelPath();
		this.modelName.setText(path != null ? new File(path).getName() : "");
		int gain = Math.round(this.processor.getGain() * 100.0f);
		this.gainSlider.setValue(gain);
		this.gainValueLabel.setText(gain + "%");
		this.updating = false;
	}

	public void updateGain() {
		if (!this.updating) {
			int value = this.gainSlider.getValue();
			this.processor.setGain(value / 100.0f);
			this.gainValueLabel.setText(value + "%");
			this.callback.onChange(false);
		}
	}

	public void close() {
		if (this.isOpen()) {
			this.dialog.dispose();
			this.dialog = null;
		}
	}

	public void focus() {
		if (this.isOpen()) {
			this.dialog.moveToTop();
		}
	}

	public boolean isOpen() {
		return (this.dialog != null && !this.dialog.isDisposed());
	}

	public void onSelect(UISelectionEvent event) {
		this.updateGain();
	}
}